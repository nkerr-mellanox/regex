/* SPDX-License-Identifier: BSD-3-Clause
 * Copyright 2019 Mellanox Technologies, Ltd
 */
#ifndef __MLX5_H__
#define __MLX5_H__

#include <infiniband/mlx5dv.h>
#include <rte_regexdev_driver.h>
#include <rte_rwlock.h>

#include "mlx5_regex_mr.h"

struct mlx5_database_ctx {
	uint32_t umem_id;
	uint64_t offset;
};

struct mlx5_regex_db {
	/* TODO: refer to struct rxp_database of Yuval code */
	void *raw_mem;
	struct mlx5dv_devx_umem *umem;
	/* TODO: refer to Yuval's struct mlx5_database_ctx */
	struct mlx5_database_ctx umem_ctx;
};

struct mlx5_regex_priv {
	struct rte_regex_dev regex_dev;
	struct ibv_context *ctx; /* Device context. */
	struct ibv_pd *pd;
	uint32_t pdn;
	uint32_t eqn;
	struct mlx5_regex_db *db_desc;
	int num_db_desc;
	struct {
		uint32_t dev_gen; /* Generation number to flush local caches. */
		rte_rwlock_t rwlock; /* MR Lock. */
		struct mlx5_mr_btree cache; /* Global MR cache table. */
		struct mlx5_mr_list mr_list; /* Registered MR list. */
		struct mlx5_mr_list mr_free_list; /* Freed MR list. */
	} mr;
	TAILQ_ENTRY(mlx5_regex_priv) next;
	struct rte_pci_device *pci_dev;
};

#endif
