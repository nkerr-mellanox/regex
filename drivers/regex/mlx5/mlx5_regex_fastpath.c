/* SPDX-License-Identifier: BSD-3-Clause
 * Copyright 2019 Mellanox Technologies, Ltd
 */
#include <unistd.h>
#include <sys/mman.h>

#include <rte_malloc.h>
#include <rte_log.h>
#include <rte_errno.h>
#include <rte_bus_pci.h>
#include <rte_pci.h>
#include <rte_regexdev_driver.h>

#include <infiniband/mlx5dv.h>
#include <mlx5_glue.h>
#include <mlx5_common.h>
#include <mlx5_prm.h>

#include "mlx5.h"
#include "mlx5_regex.h"
#include "mlx5_regex_utils.h"
#include "rxp-csrs.h"
#include "rxp-api.h"

/**
 * DPDK callback for enqueue.
 *
 * @param dev
 *   Pointer to the regex dev structure.
 * @param qp_id
 *   The queue to enqueue the traffic to.
 * @param ops
 *   List of regex ops to enqueue.
 * @param nb_ops
 *   Number of ops in ops parameter.
 *
 * @return
 *   Number of packets successfully enqueued (<= pkts_n).
 */
int
mlx5_regex_dev_enqueue(struct rte_regex_dev *dev, uint16_t qp_id,
		       struct rte_regex_ops **ops, uint16_t nb_ops)
{
	struct mlx5_regex_priv *priv = container_of(dev,
						    struct mlx5_regex_priv,
						    regex_dev);
	struct mlx5_regex_queues *queue = &priv->queues[qp_id];
 	int i;
	struct rte_regex_ops *op; 
	int sent = 0, ret = 0;

	printf(" pi = %d, ci = %d\n",  queue->pi, queue->ci);
	for (i = 0; i < nb_ops; i++) {
		if ((queue->pi - queue->ci) >= MLX5_REGEX_MAX_JOBS)
			return sent;
		op = ops[i];
		ret = rxp_submit_job(queue->handle,
			       queue->pi % MLX5_REGEX_MAX_JOBS,
			       (*op->bufs)[0]->buf_addr,
			       (*op->bufs)[0]->buf_size,
			       op->group_id0, op->group_id1, op->group_id2,
			       op->group_id3, false, false);
		if (ret) {
			printf("submit job failed err = %d\n", ret);
			return sent;
		}
		queue->jobs[queue->pi % MLX5_REGEX_MAX_JOBS].user_id =
			op->user_id; 
		queue->jobs[queue->pi % MLX5_REGEX_MAX_JOBS].used = 1;
		sent++;
		queue->pi++;
	}
	return sent;
}

/**
 * DPDK callback for dequeue.
 *
 * @param dev
 *   Pointer to the regex dev structure.
 * @param qp_id
 *   The queue to enqueue the traffic to.
 * @param ops
 *   List of regex ops to dequeue.
 * @param nb_ops
 *   Number of ops in ops parameter.
 *
 * @return
 *   Number of packets successfully dequeued (<= pkts_n).
 */
int
mlx5_regex_dev_dequeue(struct rte_regex_dev *dev, uint16_t qp_id,
		       struct rte_regex_ops **ops, uint16_t nb_ops)
{
	struct mlx5_regex_priv *priv = container_of(dev,
						    struct mlx5_regex_priv,
						    regex_dev);
	struct mlx5_regex_queues *queue = &priv->queues[qp_id];
 	int i;
	struct rte_regex_ops *op; 
	int rec = 0;
	bool rx_ready = false;
	bool tx_ready;
	struct rxp_response *res;
	int j;
	int cnt = 0;
	int offset;


	rxp_queue_status(queue->handle, &rx_ready, &tx_ready); // resp_ready = true
	if (!rx_ready) {
		printf("should exit\n");
		return 0;
	}
	printf(" pi = %d, ci = %d\n", queue->pi, queue->ci);
	for (i = 0; i < nb_ops; i++) {
		if ((queue->pi - queue->ci) == 0) {
			printf("Queue full rec = %d, pi = %d, ci = %d\n", rec, queue->pi, queue->ci);
			return rec;
		}
		op = ops[i];
		if (cnt <= 0) {	
			cnt = rxp_read_response_batch(queue->handle,
						      &queue->resp_ctx); //resp resdy = false
			if (cnt == 0)
				return rec;
		}
		res = rxp_next_response(&queue->resp_ctx);
		cnt--;
		if (res == NULL)
			continue;

		uint32_t id = DEVX_GET(regexp_metadata, res, job_id);
		op->user_id = queue->jobs[id].user_id;
		op->nb_matches = DEVX_GET(regexp_metadata, res, match_count);
		op->nb_actual_matches = DEVX_GET(regexp_metadata, res,
						 detected_match_count);
		for (j = 0; j < op->nb_matches; j++) {
			offset = sizeof(struct rxp_response_desc) + j * (64/8);
			op->matches[j].rule_id = DEVX_GET(regexp_match_tuple, 
							  ((uint8_t *)res +
							  offset), rule_id);
			op->matches[j].offset = DEVX_GET(regexp_match_tuple, 
							  ((uint8_t *)res +
							  offset), start_ptr);
			op->matches[j].len = DEVX_GET(regexp_match_tuple, 
							  ((uint8_t *)res +
							  offset), length);
		}
		queue->jobs[i].used = 0;
		rec++;
		queue->ci++;
	}
//exit:
//	for (; queue->ci < queue->pi; queue->ci++) {
//		/*if (queue->jobs[res->header.job_id].used == 1)
//			break;*/
//	}
	printf("rec = %d\n", rec);
	return rec;
}