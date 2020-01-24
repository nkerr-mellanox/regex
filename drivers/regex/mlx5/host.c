/**
 * @file    host.c
 * @author  Titan IC Systems <support@titanicsystems.com>
 *
 * @section DESCRIPTION
 *
 *   Minimal reference application. The RXP is initialized and the rules memories
 *   are programmed using a pre-compiled ROF file. Random length jobs are created
 *   containing the string "hello world" at a random offset. These jobs are
 *   dispatched to the RXP and responses are received. The number of jobs,
 *   responses and matches are periodically displayed along with the job bit rate.
 *   The application supports multiple queues and multiple Hyperion cards. Each
 *   queue is used to communicate using a single rx/tx queue pair.
 *
 * @section LICENSE
 *
 *   BSD LICENSE
 *
 *   Copyright (C) 2020 Titan IC Systems Ltd. All rights reserved.
 *
 *   Redistribution and use in source and binary forms, with or without
 *   modification, are permitted provided that the following conditions
 *   are met:
 *
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in
 *       the documentation and/or other materials provided with the
 *       distribution.
 *     * Neither the name of Titan IC Systems Ltd. nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 *   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *   "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *   LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 *   A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 *   OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 *   SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 *   LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 *   DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 *   THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 *   (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 *   OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <getopt.h>
#include <sys/mman.h>
#include <inttypes.h>
#include <limits.h>
#include <stdbool.h>
#include <fcntl.h>
#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>


#include "rxp-csrs.h"
#include "rxp-api.h"
#include "host.h" //"mlx5_regex.h" included here

/* The maximum size of any RXP rules set */
#define MAX_SIZE_RES_DES    (sizeof(struct rxp_response_desc))
#define MAX_DB_SIZE         (1u << 27u) //128MB
#define MAX_SIZE_MATCH_RESP (254 * sizeof(struct rxp_match_tuple)) 
#define RXP_SQ_NOT_BUSY     false
#define RXP_SQ_BUSY         true

/* Global variables */
static bool debug_enabled = false;              //Print all general debug
static bool debug_csrs    = false;              //Print CSRs register

//TODO Remove rxp global
static struct rxp_mlnx_dev rxp;


/* ***************************************************************** */
/* ***************** Private Func Declarations ********************* */
/* ***************************************************************** */
static int rxp_init(uint8_t rxp_eng);
static int rxp_init_rtru(uint8_t rxp_eng);
static void rxp_disable(uint8_t rxp_eng);
static void rxp_enable(uint8_t rxp_eng);
static void rxp_dump_csrs(const char *info, uint8_t rxp_eng);
static int rxp_flush_rules(struct rxp_rof_entry *rules,
                           int count,
                           uint8_t rxp_eng);
static int rxp_write_rules_via_cp(struct rxp_rof_entry *rules,
                                  int count,
                                  uint8_t rxp_eng);
static int rxp_poll_csr_for_value(struct rxp_mlnx_dev *rxp,
                                  uint32_t *value, uint32_t address,
                                  uint32_t expected_value,
                                  uint32_t expected_mask,
                                  unsigned timeout_ms, uint8_t rxp_eng);


/* ************************************************************* */
/* ********************* Private Functions ********************* */
/* ************************************************************* */

static void rxp_dump_csrs(const char *info, uint8_t rxp_eng)
{
    uint32_t reg, i;

    if (debug_csrs)
    {
        /* Main CSRs*/
        for (i = 0; i < 31; i++)
        {
            if (mlx5_regex_register_read(rxp.device_ctx, rxp_eng,
                    RXP_CSR_BASE_ADDRESS + (RXP_CSR_WIDTH * i), &reg))
            {
                mlnx_log("Error: Failed to read Main CSRs (rxp_dump_csrs)!");
                return;
            }

            mlnx_log("RXP register (Main CSRs- %s func) (%d): %08x",
                        info ? info : "", i, reg);
        }

        /* RTRU CSRs*/
        for (i = 0; i < 31; i++)
        {
            if (mlx5_regex_register_read(rxp.device_ctx, rxp_eng,
                    RXP_CSR_BASE_ADDRESS + (RXP_CSR_WIDTH * i), &reg))
            {
                mlnx_log("Error: Failed to read RTRU CSRs (rxp_dump_csrs)!");
                return;
            }

            mlnx_log("RXP register (RTRU CSRs- %s func) (%d): %08x",
                     info ? info : "", i, reg);
        }

        /* MQ registers */
        for (i = 0; i < 31; i++)
        {
            if (mlx5_regex_register_read(rxp.device_ctx, rxp_eng,
                    RXP_STATS_CSR_BASE_ADDRESS + (RXP_CSR_WIDTH * i), &reg))
            {
                mlnx_log("Error: Failed to Stat CSRs (rxp_dump_csrs)!");
                return;
            }

            mlnx_log("RXP Stat register (Stat CSRs %s func) (%d): %08x",
                     info ? info : "", i, reg);
        }
    }
}


static void rxp_disable(uint8_t rxp_eng)
{
    uint32_t ctrl;
    int ret;

    mlnx_log("Disabling RXP");

    if ((ret = mlx5_regex_register_read(rxp.device_ctx, rxp_eng,
            RXP_CSR_CTRL, &ctrl)))
    {
        mlnx_log("rxp_disable: Error CP read failed to Disable RXP!");
        return;
    }

    ctrl &= ~RXP_CSR_CTRL_GO;

    if ((ret = mlx5_regex_register_write(rxp.device_ctx, rxp_eng,
            RXP_CSR_CTRL, ctrl)))
    {
        mlnx_log("rxp_disable: Error failed to write bytes via CP -- Returned [%d]", ret);
        return;
    }
}


static void rxp_enable(uint8_t rxp_eng)
{
    uint32_t ctrl;
    int ret;

    mlnx_log("Enabling RXP");

    if ((ret = mlx5_regex_register_read(rxp.device_ctx, rxp_eng,
            RXP_CSR_CTRL, &ctrl)))
    {
        mlnx_log("rxp_enable: Error Control Plane Read failed to enable RXP!");
        return;
    }

    ctrl |= RXP_CSR_CTRL_GO;

    if ((ret = mlx5_regex_register_write(rxp.device_ctx, rxp_eng,
            RXP_CSR_CTRL, ctrl)))
    {
        mlnx_log("rxp_enable: Error failed to write bytes via CP [Er:%d]", ret);
        return;
    }
}


static int rxp_poll_csr_for_value(struct rxp_mlnx_dev *rxp, uint32_t *value,
                uint32_t address,
                uint32_t expected_value,
                uint32_t expected_mask,
                unsigned timeout_ms,
                uint8_t rxp_eng)
{
    unsigned i;
    int ret;

    ret = -EBUSY;
    for (i = 0; i < timeout_ms; i++) 
    {
        if (mlx5_regex_register_read(rxp->device_ctx, rxp_eng, address, value))
        {
            mlnx_log("Error: Failed to poll CSR!");
            return -1;
        }

        if (debug_enabled)
        {
            mlnx_log("rxp_poll_csr_for_value: Expected: 0x%x; Actual: 0x%x",
                        expected_value, *value);
        }

        if ((*value & expected_mask) == expected_value) 
        {
            ret = i;  /* return number of cycles it took. */
            break;
        }
        usleep(1000);
    }

    return ret;
}


static int rxp_init_rtru(uint8_t rxp_eng)
{
    uint32_t ctrl_value;
    uint32_t poll_value;
    uint32_t expected_value;
    uint32_t expected_mask;
    int ret = 0;

    /* Read the rtru ctrl CSR */
    if (mlx5_regex_register_read(rxp.device_ctx, rxp_eng,
            RXP_RTRU_CSR_CTRL, &ctrl_value))
    {
        mlnx_log("Error CP read failed (init_rtru)!");
        return -1;
    }

    /* Check the rtru csr ctrl init bit. If it is set then clear it */
    if (ctrl_value & RXP_RTRU_CSR_CTRL_INIT)
    {
        ctrl_value &= ~(RXP_RTRU_CSR_CTRL_INIT);
        mlx5_regex_register_write(rxp.device_ctx, rxp_eng,
                                  RXP_RTRU_CSR_CTRL, ctrl_value);
    }

    /* Set the init bit in the rtru ctrl CSR. */
    ctrl_value |= RXP_RTRU_CSR_CTRL_INIT;
    mlx5_regex_register_write(rxp.device_ctx, rxp_eng,
                              RXP_RTRU_CSR_CTRL, ctrl_value);

    /* Clear the init bit in the rtru ctrl CSR. */
    ctrl_value &= ~(RXP_RTRU_CSR_CTRL_INIT);
    mlx5_regex_register_write(rxp.device_ctx, rxp_eng,
                              RXP_RTRU_CSR_CTRL, ctrl_value);

    /* Set the init_mode == 0 in the rtru ctrl CSR */
    //TODO - Check for Mlnx System if we need RXP to do EM init too??
    //I have removed:
    //RXP_RTRU_CSR_CTRL_INIT_MODE_IM_L1_L2_EM here!
    ctrl_value |= RXP_RTRU_CSR_CTRL_INIT_MODE_IM_L1_L2;
    mlx5_regex_register_write(rxp.device_ctx, rxp_eng,
                              RXP_RTRU_CSR_CTRL, ctrl_value);

    /* Need to sleep for a short period after pulsing the rtru init bit.  */
    usleep(20000);

    /* Poll the rtru status CSR until all the init done bits are set. */
    mlnx_log("Info: Waiting for RXP rule memory to complete init");

    /* Check that the following bits are set in the RTRU_CSR. */
    expected_value = RXP_RTRU_CSR_STATUS_IM_INIT_DONE |
            RXP_RTRU_CSR_STATUS_L1C_INIT_DONE |
            RXP_RTRU_CSR_STATUS_L2C_INIT_DONE;
            //TODO re-add following line if required to enable EM programming!
           //RXP_RTRU_CSR_STATUS_EM_INIT_DONE;
    expected_mask = expected_value;

    ret = rxp_poll_csr_for_value(&rxp, &poll_value,
                RXP_RTRU_CSR_STATUS,
                expected_value, expected_mask,
                RXP_INITIALIZATION_TIMEOUT, rxp_eng);

    if (ret < 0)
    {
        mlnx_log("Error: Rule memory not initialised: 0x%08X", poll_value);
        return ret;
    }

    mlnx_log("Info: Rule Memory took %d cycles to initialise: 0x%08X",
             ret, poll_value);

    /* Clear the init bit in the rtru ctrl CSR */
    ctrl_value &= ~(RXP_RTRU_CSR_CTRL_INIT);
    mlx5_regex_register_write(rxp.device_ctx, rxp_eng,
                              RXP_RTRU_CSR_CTRL, ctrl_value);

    return 0;
}


static int rxp_write_rules_via_cp(struct rxp_rof_entry *rules,
                                  int count, uint8_t rxp_eng)
{
    int i;
    uint32_t tmp;

    for (i = 0; i < count; i++) 
    {
        tmp = (uint32_t)rules[i].value;
        mlx5_regex_register_write(rxp.device_ctx, rxp_eng,
                                  RXP_RTRU_CSR_DATA_0, tmp);
        tmp = (uint32_t)(rules[i].value >> 32);
        mlx5_regex_register_write(rxp.device_ctx, rxp_eng,
                                  RXP_RTRU_CSR_DATA_0 + RXP_CSR_WIDTH, tmp);
        tmp = rules[i].addr;
        mlx5_regex_register_write(rxp.device_ctx, rxp_eng,
                                  RXP_RTRU_CSR_ADDR, tmp);
    }

    return 0;
}


static int rxp_flush_rules(struct rxp_rof_entry *rules,
                           int count, uint8_t rxp_eng)
{
    uint32_t val;
    uint32_t fifo_depth;
    int ret;

    /* Write down instruction to CSRs via control plane */
    ret = rxp_write_rules_via_cp(rules, count, rxp_eng);

    if (ret < 0)
    {
        mlnx_log("Error: rxp_write_rules failed, 0x%x", ret);
        return -1;
    }

    if ((ret = mlx5_regex_register_read(rxp.device_ctx, rxp_eng,
                RXP_RTRU_CSR_CAPABILITY, &fifo_depth)))
    {
        mlnx_log("Error Control Plane read failed (rxp_flush_rules)!");
        return -1;
    }

    ret = rxp_poll_csr_for_value(&rxp, &val,
                RXP_RTRU_CSR_FIFO_STAT,
                count, ~0,
                RXP_POLL_CSR_FOR_VALUE_TIMEOUT, rxp_eng);
    if (ret < 0)
    {
        mlnx_log("Error: Rules not received by RXP: credit: %d, depth: %d",
                    val, fifo_depth);
        return ret;
    }

    mlnx_log("Info: RTRU FIFO depth: 0x%x", fifo_depth);
    mlnx_log("Info: Rules flush took %d cycles.", ret);

    if ((ret = mlx5_regex_register_read(rxp.device_ctx, rxp_eng,
            RXP_RTRU_CSR_CTRL, &val)))
    {
        mlnx_log("Error Control Plane read failed (rxp_flush_rules)!");
        return -1;
    }

    val |= RXP_RTRU_CSR_CTRL_GO;

    ret = mlx5_regex_register_write(rxp.device_ctx, rxp_eng,
                                    RXP_RTRU_CSR_CTRL, val);

    ret = rxp_poll_csr_for_value(&rxp, &val,
                RXP_RTRU_CSR_STATUS,
                RXP_RTRU_CSR_STATUS_UPDATE_DONE,
                RXP_RTRU_CSR_STATUS_UPDATE_DONE,
                RXP_POLL_CSR_FOR_VALUE_TIMEOUT, rxp_eng);

    if (ret < 0)
    {
        mlnx_log("Info: Rules update timeout: 0x%08X", val);
        return ret;
    }

    mlnx_log("Info: Rules update took %d cycles", ret);

    if (mlx5_regex_register_read(rxp.device_ctx, rxp_eng,
            RXP_RTRU_CSR_CTRL, &val))
    {
        mlnx_log("Error Control plane read failed (rxp_flush_rules)!");
        return -1;
    }

    val &= ~(RXP_RTRU_CSR_CTRL_GO);

    if (mlx5_regex_register_write(rxp.device_ctx, rxp_eng,
            RXP_RTRU_CSR_CTRL, val))
    {
        mlnx_log("Error: Control Plane write failed (rxp_flush_rules)!");
        return -1;
    }

    if (debug_enabled)
    {
        mlnx_log("rxp_flush_rules: Finished");
    }

    return 1;
}


static int rxp_init(uint8_t rxp_eng)
{
    int ret;
    uint32_t reg;

    /* Clear the init bit if set */
    if (mlx5_regex_register_read(rxp.device_ctx, rxp_eng, RXP_CSR_CTRL, &reg))
    {
        mlnx_log("Error: Control Plane read failed (rxp_init)!");
        return -1;
    }

    if (reg & RXP_CSR_CTRL_INIT)
    {
        reg &= ~RXP_CSR_CTRL_INIT;
        mlx5_regex_register_write(rxp.device_ctx, rxp_eng, RXP_CSR_CTRL, reg);
    }

    /* Pulse the init bit */
    reg |= RXP_CSR_CTRL_INIT;
    mlx5_regex_register_write(rxp.device_ctx, rxp_eng, RXP_CSR_CTRL, reg);

    reg &= ~RXP_CSR_CTRL_INIT;
    mlx5_regex_register_write(rxp.device_ctx, rxp_eng, RXP_CSR_CTRL, reg);

    /* Wait for the RXP to init */
    usleep(20000);

    /* Wait for status init bit to be set */
    ret = rxp_poll_csr_for_value(&rxp, &reg,
                RXP_CSR_STATUS,
                RXP_CSR_STATUS_INIT_DONE,
                RXP_CSR_STATUS_INIT_DONE,
                RXP_INITIALIZATION_TIMEOUT, rxp_eng);

    if (ret < 0)
    {
        mlnx_log("Error: RXP not initialised: 0x%08X (rxp_init)!", reg);
        return ret;
    }

    mlnx_log("Info: RXP took %d cycles to initialise", ret);

    /* Clear init bit again */
    if (mlx5_regex_register_read(rxp.device_ctx, rxp_eng, RXP_CSR_CTRL, &reg))
    {
        mlnx_log("Error CP read init bit failed!");
        return -1;
    }

    reg &= ~RXP_CSR_CTRL_INIT;
    mlx5_regex_register_write(rxp.device_ctx, rxp_eng, RXP_CSR_CTRL, reg);

    ret = 0;

    /*
     * Experimentation has shown that we need to run the rtru
     * initialisation twice.
     * Otherwise, we are experiencing massively degraded
     * performance with larger rule sets.
     * This is a temporary workaround so that we can continue
     * testing while the problem is analyzed from a firmware
     * perspective.
     */
    rxp_init_rtru(rxp_eng);
    ret = rxp_init_rtru(rxp_eng);

    if (ret >= 0)
    {
        /* Read + write max matches + DDOS information */
        if (mlx5_regex_register_read(rxp.device_ctx, rxp_eng,
                RXP_CSR_CAPABILITY_5, &reg))
        {
            mlnx_log("Error: CP read failed (rxp_init)!");
            return -1;
        }

        uint32_t tmp;
        mlnx_log("Info: Max matches: %d, DDOS threshold: %d",
                 reg >> 16, reg & 0xFFFF);
        tmp = (reg >> 16);
        mlx5_regex_register_write(rxp.device_ctx, rxp_eng,
                                  RXP_CSR_MAX_MATCH, tmp);
        tmp = (reg & 0xFFFF);
        mlx5_regex_register_write(rxp.device_ctx, rxp_eng,
                                  RXP_CSR_MAX_PREFIX, tmp);

        /* Zero max latency and max primary threads. */
        tmp = 0;
        ret |= mlx5_regex_register_write(rxp.device_ctx, rxp_eng,
                                         RXP_CSR_MAX_LATENCY, tmp);
        ret |= mlx5_regex_register_write(rxp.device_ctx, rxp_eng,
                                         RXP_CSR_MAX_PRI_THREAD, tmp);

        if (ret)
        {
            mlnx_log("Error: Control Plane read failed (rxp_init)!");
            return -1;
        }
    }

    return ret;
}


/* ************************************************************* */
/* ********************* Global Functions ********************** */
/* ************************************************************* */


#if MLNX_LOGGING_ENABLED
void mlnx_log(const char *fmt, ...)
{
    va_list args;

    va_start(args, fmt);
    vprintf(fmt, args);
    printf("\n");
    va_end(args);
}
#endif


uint32_t mlnx_csr_write(uint32_t *value, uint32_t csr_addr_offset,
                        uint8_t rxp_eng)
{
    int ret;

    if ((ret = mlx5_regex_register_write(rxp.device_ctx, rxp_eng,
            csr_addr_offset, *value)))
    {
        mlnx_log("Error: Failed to write bytes via CP - RXP Eng [%d] Err [%d]",
                 rxp_eng, ret);
        return -1;
    }

    return 0;
}


int mlnx_csr_read(uint32_t csr_addr_offset, uint32_t *returnVal,
                    uint8_t rxp_eng)
{
    int ret;

    if ((ret = mlx5_regex_register_read(rxp.device_ctx, rxp_eng,
            csr_addr_offset, returnVal)))
    {
        mlnx_log("Error: Failed to read bytes from CP RXP Engine [%d] Err [%d]",
                 rxp_eng, ret);
        return -1;
    }

    return 0;
}


int mlnx_write_rules(struct rxp_ctl_rules_pgm *rules, uint32_t count,
                     uint8_t rxp_eng)
{
    unsigned int pending;
    uint32_t block, reg, val, rule_cnt, rule_offset, rtru_max_num_entries;
    int ret;

    if (count < sizeof(struct rxp_ctl_hdr))
    {
        return -EINVAL;
    }

    //TODO possibly add mutex_lock in here for new Mlnx API as multiple apps
    //may attempt to program RXPs!

    /* For a non-incremental rules program, re-init the RXP */
    if (rules->hdr.cmd == RXP_CTL_RULES_PGM)
    {
        ret = rxp_init(rxp_eng);
        if (ret < 0)
        {
            return ret;
        }
    }

    if (rules->count == 0)
    {
        return -EINVAL;
    }

    /* Confirm the RXP is initialised */
    if ((ret = mlx5_regex_register_read(rxp.device_ctx, rxp_eng,
            RXP_CSR_STATUS, &val)))
    {
        mlnx_log("Error: Failed to read bytes from RXP engine [%d] - Err [%d]",
                 rxp_eng, ret);
        return -1;
    }

    if (!(val & RXP_CSR_STATUS_INIT_DONE))
    {
        mlnx_log("Info: RXP not initialised: 0x%08X (mlnx_write_rules)", val);
        return -EBUSY;
    }

    /* Get the RTRU maximum number of entries allowed. */
    if ((ret = mlx5_regex_register_read(rxp.device_ctx, rxp_eng,
                RXP_RTRU_CSR_CAPABILITY, &rtru_max_num_entries)))
    {
        mlnx_log("Error: Failed to read RTRU Cap. RXP Engine [%d] Er[%d]",
                    rxp_eng, ret);
        return -1;
    }

    /* RTRU num entries in the first 16bits of capabilities */
    rtru_max_num_entries = (rtru_max_num_entries & 0x00FF);


/*
 * ****************************************************************************
 * *** TODO: Change below as need to be able to copy RXP_ROF_ENTRY_EM
 * *** instructions to MLNX shared memory...
 * ***      1. CSR internal instructions only. 2) CRS and External programming                              * *** Note: We need to strip out external instructions if external programming.
 * ****************************************************************************
 */

    rule_cnt = 0;
    pending = 0;
    while (rules->count > 0)
    {
        if ((rules->rules[rule_cnt].type == RXP_ROF_ENTRY_INST) ||
            (rules->rules[rule_cnt].type == RXP_ROF_ENTRY_IM) ||
            (rules->rules[rule_cnt].type == RXP_ROF_ENTRY_EM)) 
        {
            pending++;
            rule_cnt++;

            /*
             * If we're parsing the last rule, or if we've reached the maximum
             * number of rules for this batch, then flush the rules batch to
             * the RXP.
             */
            if (rules->count == 1 || pending == rtru_max_num_entries)
            {
                rule_offset = (rule_cnt - pending);

                ret = rxp_flush_rules(&rules->rules[rule_offset], pending,
                                      rxp_eng);

                if (!ret)
                {
                    mlnx_log("Error: CP read failed (flush_rules)!");
                    return ret;
                }
                pending = 0;
            }
        }
        else if ((rules->rules[rule_cnt].type == RXP_ROF_ENTRY_EQ) ||
            (rules->rules[rule_cnt].type == RXP_ROF_ENTRY_GTE) ||
            (rules->rules[rule_cnt].type == RXP_ROF_ENTRY_LTE) ||
            (rules->rules[rule_cnt].type == RXP_ROF_ENTRY_CHECKSUM) ||
            (rules->rules[rule_cnt].type == RXP_ROF_ENTRY_CHECKSUM_EX_EM))
        {
            if (pending)
            {
                /* Flush rules before checking reg values */
                rule_offset = (rule_cnt - pending);
                ret = rxp_flush_rules(&rules->rules[rule_offset], pending,
                                      rxp_eng);

                if (!ret)
                {
                    mlnx_log("Error: CP read failed (flush_rules)!");
                    return ret;
                }
            }

            block = (rules->rules[rule_cnt].addr >> 16) & 0xFFFF;
            if (block == 0)
                reg = RXP_CSR_BASE_ADDRESS;
            else if (block == 1)
                reg = RXP_RTRU_CSR_BASE_ADDRESS;
            else
            {
                mlnx_log("Error: Invalid ROF register 0x%08X!",
                         rules->rules[rule_cnt].addr);
                return -EINVAL;
            }

            reg += (rules->rules[rule_cnt].addr & 0xFFFF) * RXP_CSR_WIDTH;

            ret = mlx5_regex_register_read(rxp.device_ctx, rxp_eng, reg, &val);

            if (!ret)
            {
                mlnx_log("Error: CP read failed (FR) RXP Engine [%d]-Err [%d]!",
                         rxp_eng, ret);
                return ret;
            }

            if (((rules->rules[rule_cnt].type == RXP_ROF_ENTRY_EQ) ||
                (rules->rules[rule_cnt].type == RXP_ROF_ENTRY_CHECKSUM))
                && (val != rules->rules[rule_cnt].value))
            {
                mlnx_log("Info: Unexpected value for reg %x" PRIu32
                            ", got %x" PRIu32 ", expected %" PRIx64 ".",
                            rules->rules[rule_cnt].addr, val,
                            rules->rules[rule_cnt].value);
                return -EINVAL;
            }
            else if ((rules->rules[rule_cnt].type == RXP_ROF_ENTRY_GTE) &&
                (val < rules->rules[rule_cnt].value))
            {
                mlnx_log("Info: Unexpected value reg 0x%08X, got %X, expected >= %" PRIx64 ".",
                         rules->rules[rule_cnt].addr, val,
                         rules->rules[rule_cnt].value);
                return -EINVAL;
            }
            else if ((rules->rules[rule_cnt].type == RXP_ROF_ENTRY_LTE) &&
                (val > rules->rules[rule_cnt].value))
            {
                mlnx_log("Info: Unexpected value reg 0x%08X, got %08X, expected <= %" PRIx64
                        ".", rules->rules[rule_cnt].addr, val,
                         rules->rules[rule_cnt].value);
                return -EINVAL;
            }

            rule_cnt++;
            pending = 0;
        }
        else
        {
            mlnx_log("Error: Invalid rule type %d!",
                        rules->rules[rule_cnt].type);
            return -EINVAL;
        }

        rules->count--;
    }

    if (debug_enabled)
    {
        mlnx_log("Press Enter key to continue: (Rules Programmed)");
        getchar();
    }

    if (debug_enabled)
    {
        rxp_dump_csrs("Rules Programmed", rxp_eng);
    }

    return ret;
}


/*
 * Function to retrieve the response header/s and matches from Mlnx CQueues
 * TODO Notably Mellanox said we will not have to manage alignment of resp!
 * Remove this point after initial testing success!
 */
int mlnx_read_resp(struct rxp_queue *rxp_queue, uint8_t *buf, size_t buf_size,
                   uint32_t *num_returned_resp)
{
    int ret;
    int match_count = 0;
    uint32_t i;
    uint32_t tmp_match_len, total_response_len = 0, response_len; //Len in bytes

    /* Ensure clear before starting to count! */
    *num_returned_resp = 0;

    /* Extract all the responses into applications buffer */
    for (i = 0; i < NUM_SQS; i++)
    {
        if (rxp_queue->sq_buf[i].sq_resp_ready == true)
        {
            /* Must have response in Buffer, so copy into application buffer */
            //TODO: where is regexp_metadata declared...?
            //      Possibly add to sq_buff structure.
            match_count = DEVX_GET(regexp_metadata,
                                rxp_queue->sq_buf[i].metadata_p, match_count);

            if (match_count < 0)
            {
                /*
                 * TODO check if this is possible?
                 * i.e. a return value of less than 0?
                 */
                ret = -1;
                goto early_exit;
            }

            /*
             * Check that we have enough space in app buffer to copy
             * response+matche/s?
             */
            //TODO ensure create new Match Tuple of 64bits!
            tmp_match_len = match_count * (sizeof(struct rxp_match_tuple));
            response_len = tmp_match_len + sizeof(struct rxp_response_desc);

            if ((total_response_len + response_len) > buf_size)
            {
                /* Going to overflow app buffer so do not copy */
                //TODO: can you do a double DEVX_GET of same CQ?
                //      or if this warning happens I'll have to store response??
                mlnx_log("Warning: Response read too big! [Buf Size=%d; Read Size=%d]!",
                         buf_size, (total_response_len + response_len));
                break;
            }

            /* Continue to copy response data to app */
            //TODO: check Endianess here?
            //      As may need to read from regexp_metadata struct?
            memcpy(buf, rxp_queue->sq_buf[i].metadata_p,
                    sizeof(struct rxp_response_desc));

            //TODO: Yuval is this correct way to get output_p or we need to do DEVX_get?
            //TODO check Endianess here? As may need to read from regexp_metadata struct?
            memcpy(&buf[MAX_SIZE_RES_DES], rxp_queue->sq_buf[i].output_p,
                    tmp_match_len);

            /*
             * Clear ready for next submit of job (As dont want to
             * double read this!)
             */
            rxp_queue->sq_buf[i].sq_resp_ready = false;
            rxp_queue->sq_buf[i].sq_busy = RXP_SQ_NOT_BUSY;

            (*num_returned_resp)++;
            total_response_len += response_len;
        }
    }

    if (debug_enabled)
    {
        /* Display all bytes of the read response data */
        for(i = 0; i < total_response_len; i++)
        {
            mlnx_log("Info: mlnx_read_resp: Read Buffer results: [%d]:[%d]",
                     i, buf[i]);
        }
    }

    return total_response_len;

early_exit:
    return ret;
}


/*
 * Desc: Submitting jobs to Mellanox Queues...
 *     void *input_p;      //Jobs data
 *     void *output_p;     //response match
 *     void *metadata_p;   ///Response header data
 */
size_t mlnx_submit_job(struct rxp_queue *rxp_queue,
                       struct rxp_mlnx_job_desc *data,
                       uint16_t job_count)
{
    int bytes_written = 0;
    uint8_t tmp_ctrl;
    struct rxp_job_desc *job;
    uint16_t joblen = 0;
    uint32_t i, num_jobs_processed = 0;

    /* Check if user trying to submit more jobs than SQs? */
    if (job_count >= NUM_SQS)
    {
        //TODO possibly remove this after initial development testing...
        mlnx_log("Warning: Attempt to transmit more jobs than queues!");

        //Note, dont need to Error here, as can simply restrict...
		//warning above enough to catch for now!
    }

    if (job_count == 0)
    {
        return -1;
    }

    /* Search through buffers for empty SQ...*/
    for(i = 0; i < NUM_SQS; i++)
    {
        if (num_jobs_processed >= job_count)
        {
            /* Exit as no more jobs to send */
            break;
        }

        if ((rxp_queue->sq_buf[i].sq_busy == RXP_SQ_NOT_BUSY) &&
                (rxp_queue->sq_buf[i].sq_resp_ready != true))
        {
            /* Must have an empty buffer to store new job...{YEH} */
            /* Copy data into local buffer ready to send to RXP  */
            /*
             * Mellanox will be taking care of the Padding/Alignment
             * of jobs for RXP FIFOs
             */

            /*
             * First thing to copy CTRL segment into buffer
             * As App sending 16bit CTRL field we need to truncate into 4bits
             * to pass to ctrl_set function below
             */
            job = (struct rxp_job_desc *)(data[num_jobs_processed * 2].data_ptr);
            joblen = data[num_jobs_processed * 2].len;

            //TODO: For Yuval: I still dont know 100% how you want me to
            //      figure this control field out! (If actually only 1bit,
            //      then I can change!)
            tmp_ctrl  = job->ctrl & 0x000C; //Get 1st 2 bits needed from Ctrl
            tmp_ctrl |= job->ctrl & 0x0100; //8th bit needed

            /*
             * TODO: (NOTE) As not sure how Mlnx is handling the JOBID's I'm
             * going to store each JobId locally to each SQ for later
             * retrieval/mapping in response
             */
            rxp_queue->sq_buf[i].job_id = job->job_id;

            /*
             * Note: jobid going into metadata, joblen taken from input data seg
             * and ctrl field mapped above
             */
            //TODO: NOTE:  consider Endiness as doing memcpy instead of DEVX...?
            mlx5_regex_set_ctrl_seg(&rxp_queue->sq_buf[i].ctrl_seg, 0,
                                    job->subset, tmp_ctrl);

            /* Copy job data into input ptr */
            memcpy(rxp_queue->sq_buf[i].input_p,
                        (uint8_t*)data[num_jobs_processed * 2 + 1].data_ptr,
                        joblen);

            mlx5dv_set_data_seg(&rxp_queue->sq_buf[i].input_seg, joblen,
                                mlx5_regex_get_lkey(
                                rxp_queue->sq_buf[i].input_buff),
                                (uintptr_t)rxp_queue->sq_buf[i].input_p);

            mlx5dv_set_data_seg(&rxp_queue->sq_buf[i].output_seg,
                                MAX_SIZE_MATCH_RESP,
                                mlx5_regex_get_lkey(
                                    rxp_queue->sq_buf[i].output_buff),
                                (uintptr_t)rxp_queue->sq_buf[i].output_p);

            /* Return work_id, or -1 in case of err */
            rxp_queue->sq_buf[i].work_id = mlx5_regex_send_work(
                                rxp_queue->rxp_job_ctx,
                                &rxp_queue->sq_buf[i].ctrl_seg,
                                mlx5_regex_get_lkey(
                                    rxp_queue->sq_buf[i].metadata_buff),
                                &rxp_queue->sq_buf[i].input_seg,
                                &rxp_queue->sq_buf[i].output_seg, i);

            if (rxp_queue->sq_buf[i].work_id > -1)
            {
                rxp_queue->sq_buf[i].sq_busy = true; //Queue now in use!

                /* Job head and bytes only count, not including padding here */
                bytes_written += joblen + sizeof(struct rxp_job_desc);

                /* Keep account of the total number of jobs processed */
                num_jobs_processed++;
            }
            else
            {
                /* Error with Job transmission */
                //TODO: How best to cope with this error? Maybe dont exit,
                //      simply return number of jobs sent!!!!
                if (debug_enabled)
                {
                    mlnx_log("mlnx_submit_job: Failed to send job [%d]!", i);
                }
                break;
            }
        }
    }

    if (debug_enabled)
    {
        mlnx_log("mlnx_submit_job: Finished!");
    }

    return bytes_written;
}


/*
 * mlnx_poll: This function must check if any responses are available for queue.
 *            It must also determine if any other work queues available to send
 *            further jobs.
 *
 *          - If have more queue available to send jobs then tx_ready = true
 *          - If polling function returns success (i.e. 1) then some job
 *            response has returned so return rx_ready = true
 *          mlx5_regex_poll(): Return 1 in case of success, -1 in case of error,
 *          0 if no completion
 */
int mlnx_poll(struct rxp_queue *rxp_queue, bool *rx_ready, bool *tx_ready)
{
    int ret = 1, i;

    if (rx_ready != NULL)
    {
        *rx_ready = false;

        /*
         * Poll all SQ per RegRXP context, noting that only 1job per SQ, to
         * ensure out of order response sequence possibility
         */
        for (i = 0; i < NUM_SQS; i++)
        {
            //TODO: Check if can simply use work_id = "i", not sure if work_id
            //      maps SQ's if only 1 job per SQ!?
            ret = mlx5_regex_poll(rxp_queue->rxp_job_ctx, i,
                                   rxp_queue->sq_buf[i].work_id);

            /* 1 = response waiting, 0 = no completion, -1 = error */
            if (ret > 0)
            {
                /* Must be a response in response queue */
                *rx_ready = true;
                rxp_queue->sq_buf[i].sq_resp_ready = true;

                rxp_queue->num_resp_to_read++;

                /*
                 * TODO: Test for EARLY EXIT here, as could return to inform
                 * app to read ASAP or read all SQ first for potential response
                 * to enable batch reads!...?
                 */
                 //break;
            }
            else if (ret == 0)
            {
                /* Clear any old values */
                rxp_queue->sq_buf[i].sq_resp_ready = false;
            }
            else
            {
                /* Must be error (!TODO How best to manage a POLL error!?) */
                goto early_exit;
            }
        }
    }

    if (tx_ready != NULL)
    {
        *tx_ready = false;

        for (i = 0; i < NUM_SQS; i++)
        {
            if (rxp_queue->sq_buf[i].sq_busy == false)
            {
                //TODO: Could loop all and return number free...
                *tx_ready = true;
            }
        }
    }

    ret = 1;

early_exit:
    return ret;
}


/*
 * mlnx_open:
 * This function will open a RXP context and setup X number of SQs to allow
 * 1 job to be sent on each SQ created.  Sending 1 job per SQ allows for an out
 * of order RXP response.  This function allows create X number of SQ memory
 * ptr buffers for each SQ.
 */
int mlnx_open(struct rxp_queue *queue)
{
    unsigned int q, i, j;

    pthread_mutex_lock(&(rxp.lock));

    /* Only allow as many clients as queues */
    if (rxp.open_queues >= RXP_NUM_QUEUES)
    {
        mlnx_log("Failed to open rxp (cnt=%d)!", rxp.open_queues);
        pthread_mutex_unlock(&(rxp.lock));
        return -EBUSY;
    }

    /* Find the first available queue */
    for (q = 0; (q < RXP_NUM_QUEUES) && (rxp.queues_active & (1 << q)); q++);

    rxp.queues_active |= (1 << q);

    queue[q].rxp = &rxp;
    queue[q].q_id = q;

    /*
     * Can open up multip regex devices per thread/application.
     * Note: This open regex is not related to the physcial number of
     * RXP engines.
     */
    queue[q].rxp_job_ctx = mlx5_regex_device_open(rxp.device_ctx, NUM_SQS);

    if (!queue[q].rxp_job_ctx)
    {
        mlnx_log("Platform Info: Error opening regex device!");
        goto tidyup;
    }

    /* Note: RXP Memory alignment managed by GGA-reported by Dotan in Mlnx Q&A*/

    /*
     * As NUM_SQS required we need to setup 1 job per SQ.
     * Doing this allows for out of order responses!
     * Therefore creating a seperate memory segments per Job/SQ
     */
    for (i = 0; i < NUM_SQS; i++)
    {
        /* Alloc memory for Job/s data */
        queue[q].sq_buf[i].input_p = malloc(RXP_MAX_JOB_LENGTH);

        if (!queue[q].sq_buf[i].input_p)
        {
            mlnx_log("Error: Failed to create input buffer!");
            goto tidyup_regex_ctx;
        }

        /* Alloc Response matches memory */
        queue[q].sq_buf[i].output_p = malloc(MAX_SIZE_MATCH_RESP);

        if (!queue[q].sq_buf[i].output_p)
        {
            mlnx_log("Error: Failed to create output buffer!");
            goto tidyup_regex_ctx;
        }

        /* Alloc memory for Response header data */
        queue[q].sq_buf[i].metadata_p = malloc(sizeof(struct rxp_response_desc));

        if (!queue[q].sq_buf[i].metadata_p)
        {
            mlnx_log("Error: Failed to create metadata buffer!");
            goto tidyup_regex_ctx;
        }

        /*
         * Now we need to register each of the memories - Diff from database
         * memory setup (new Mlnx Update 20Dec-2019)
         */
        queue[q].sq_buf[i].input_buff = mlx5_regex_reg_buffer(
                        queue[q].rxp_job_ctx, queue[q].sq_buf[i].input_p,
                        RXP_MAX_JOB_LENGTH);

        if (!queue[q].sq_buf[i].input_buff)
        {
            mlnx_log("Error: Failed to register input memory!");
            goto tidyup_regex_ctx;
        }

        queue[q].sq_buf[i].output_buff = mlx5_regex_reg_buffer(
                        queue[q].rxp_job_ctx, queue[q].sq_buf[i].output_p,
                        MAX_SIZE_MATCH_RESP);

        if (!queue[q].sq_buf[i].output_buff)
        {
            mlnx_log("Error: Failed to register output memory!");
            goto tidyup_regex_ctx;
        }

        queue[q].sq_buf[i].metadata_buff = mlx5_regex_reg_buffer(
                        queue[q].rxp_job_ctx, queue[q].sq_buf[i].metadata_p,
                        sizeof(struct rxp_response_desc));

        if (!queue[q].sq_buf[i].metadata_buff)
        {
            mlnx_log("Error: Failed to register metadata memory!");
            goto tidyup_regex_ctx;
        }

        queue[q].sq_buf[i].sq_busy = RXP_SQ_NOT_BUSY;
        queue[q].sq_buf[i].sq_resp_ready = false;
        queue[q].sq_buf[i].work_id = 0;
        queue[q].sq_buf[i].job_id = 0; //JOBID should never equal 0!

    }

    if (rxp.open_queues++ == 0)
    {
        /* Enable RXP processing if were the first open */
        rxp_enable(0);
        rxp_enable(1);
    }

    if (debug_csrs)
    {
        rxp_dump_csrs("Mlnx_Open", 0);
        rxp_dump_csrs("Mlnx_Open", 1);
    }

    pthread_mutex_unlock(&(rxp.lock));

    return q;

tidyup_regex_ctx:
    for (j = 0; j < i; j++)
    {
        if (queue[q].sq_buf[j].input_p != NULL)
        {
            free(queue[q].sq_buf[j].input_p);
        }

        if (queue[q].sq_buf[j].output_p != NULL)
        {
            free(queue[q].sq_buf[j].output_p);
        }

        if (queue[q].sq_buf[j].metadata_p != NULL)
        {
            free(queue[q].sq_buf[j].metadata_p);
        }

        if (!queue[q].sq_buf[j].input_buff)
        {
            //TODO how to deregister this now with Yuvals new code as he has TODO's in API
            //mlx5dv_devx_umem_dereg(queue[q].sq_buf[j].input_umem);
        }

        if (!queue[q].sq_buf[j].output_buff)
        {
            //TODO how to deregister this now with Yuvals new code as he has TODO's in API
            //mlx5dv_devx_umem_dereg(queue[q].sq_buf[j].output_umem);
        }

        if (!queue[q].sq_buf[j].metadata_buff)
        {
            //TODO how to deregister this now with Yuvals new code as he has TODO's in API
            //mlx5dv_devx_umem_dereg(queue[q].sq_buf[j].metadata_umem);
        }
    }

tidyup:
    rxp.queues_active &= ~(1 << q);
    pthread_mutex_unlock(&(rxp.lock));

    return -1;
}


int mlnx_release(struct rxp_queue *queue)
{
    int j;

    /* Ensure all commands are flushed */
    //TODO check that all jobs/responses flushed!!???

    if (debug_csrs)
    {
        rxp_dump_csrs("Mlnx_Release", 0);
        rxp_dump_csrs("Mlnx_Release", 1);
    }

    for (j = 0; j < NUM_SQS; j++)
    {
        free(queue->sq_buf[j].input_p);
        free(queue->sq_buf[j].output_p);
        free(queue->sq_buf[j].metadata_p);
        //TODO how to deregister this now with Yuvals new code as he has TODO's in API
        //mlx5dv_devx_umem_dereg(queue->sq_buf[j].input_umem);
        //mlx5dv_devx_umem_dereg(queue->sq_buf[j].output_umem);
        //mlx5dv_devx_umem_dereg(queue->sq_buf[j].metadata_umem);
    }

    pthread_mutex_lock(&(queue->rxp->lock));
    queue->rxp->queues_active &= ~(1 << queue->q_id);

    /* Release this regex context */
    mlx5_regex_device_close(queue->rxp_job_ctx);

    if (--queue->rxp->open_queues == 0)
    {
        /* Disable RXP processing if were the last close */
        /* Note in this system there are 2 RXP Engines to shutdown! */
        rxp_disable(0);
        rxp_disable(1);
        mlnx_close(queue->rxp);
    }
    pthread_mutex_unlock(&(queue->rxp->lock));

    if (queue->rxp->open_queues == 0)
    {
        pthread_mutex_destroy(&(queue->rxp->lock));
    }

   // free(queue);
    return 1;
}


/*
 * Close any specific Mlnx functions
 * Note: Should only call this function if last application/queue in the system!
 */
int mlnx_close(struct rxp_mlnx_dev *rxp)
{
    munmap(rxp->rxp_db_desc[0].database_ptr, MAX_DB_SIZE);
    munmap(rxp->rxp_db_desc[1].database_ptr, MAX_DB_SIZE);
    mlx5dv_devx_umem_dereg(rxp->rxp_db_desc[0].db_umem);
    mlx5dv_devx_umem_dereg(rxp->rxp_db_desc[1].db_umem);
    //mlx5_free_context(rxp->device_ctx); //No longer in Mlnx API try:
    free(rxp->device_ctx); //TODO check if this is right
    ibv_free_device_list(rxp->dev_list);

    return 1;
}

/* mlnx_resume_rxp: This function is used to Resume RXP engine */
int mlnx_resume_rxp(uint8_t rxp_eng)
{
    mlx5_regex_engine_go(rxp.device_ctx, rxp_eng);

    return 1;
}

int mlnx_set_database(uint8_t rxp_eng)
{
    rxp.rxp_db_desc[rxp_eng].db_ctx.umem_id =
                                    rxp.rxp_db_desc[rxp_eng].db_umem->umem_id;
    rxp.rxp_db_desc[rxp_eng].db_ctx.offset = 0,

    /* Inform Mellanox HW where the database is pointing to in shared memory */
    mlx5_regex_database_set(rxp.device_ctx, rxp_eng /*engine_id*/,
                            &rxp.rxp_db_desc[rxp_eng].db_ctx);

    return 1;
}


/*
 * mlnx_update_database: This function will stop the RXP engine and setup
 * pointer to RXP ruleset/database (TODO: Note: Pending Mlnx responses to what
 * else database_set does?)
 */
int mlnx_update_database(uint8_t rxp_eng)
{
    /*
     * Stop RXP before doing any programming. Command will return when RXP
     * engine is idle.
     */
    mlx5_regex_engine_stop(rxp.device_ctx, rxp_eng);

    mlnx_set_database(rxp_eng);

    return 1;
}

/*
 * mlnx_init: This function will setup Card ready for programming both RXPs
 *  - So it will check for capabilities
 *  - Open a device
 *  - Setup relevant structures ready for Program Database/Ruleset for RXP
 *  - TODO:Inform Mellanox of correct Database to use.
 *
 */
int mlnx_init(void)
{
    int err;
    uint32_t fpga_ident = 0;
    int supported;
    int devn = 0;
    struct mlx5dv_context_attr attr = {0};

    if (pthread_mutex_init(&(rxp.lock), NULL) != 0)
    {
        mlnx_log("\nMutex init failed!");
        return -1;
    }

    rxp.dev_list = ibv_get_device_list(&rxp.num_devices);

    if (rxp.num_devices == 0)
    {
        mlnx_log("Platform Info: No devices found!\n");
        goto tidyup;
    }

    for (int i = 0; i < rxp.num_devices; i++)
    {
        mlnx_log("Platform Info: Device Name           : %s",
                 ibv_get_device_name(rxp.dev_list[i]));
        mlnx_log("Platform Info: Device GUID           : %d",
                 ibv_get_device_guid(rxp.dev_list[i]));
        mlnx_log("Platform Info: Device Type           : %d",
                 rxp.dev_list[i]->node_type);
        mlnx_log("Platform Info: Device Transport Type : %d",
                 rxp.dev_list[i]->transport_type);
    }

    attr.flags = MLX5DV_CONTEXT_FLAGS_DEVX;

    if (!mlx5dv_is_supported(rxp.dev_list[devn]))
    {
        mlnx_log("Platform Info: Devx not supported!");
        goto tidyup;
    }

    rxp.device_ctx = mlx5dv_open_device(rxp.dev_list[devn], &attr);
    if (!rxp.device_ctx)
    {
        mlnx_log("Platform Info: Failed to open device %s", strerror(errno));
        goto tidyup;
    }

    //TODO Check if mlx5dv_open_device needs a corresponding close_device call if fail below???

    supported = mlx5_regex_is_supported(rxp.device_ctx);

    if (!supported)
    {
        mlnx_log("Regexp not supported");
        goto tidyup_context;
    }

    /*
     * Setup database 1 for first RXP engine 0 (RXP 0)
     * Setup huge pages for rule set
     */
    rxp.rxp_db_desc[0].database_ptr = mmap(NULL, MAX_DB_SIZE, PROT_READ |
            PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS | MAP_POPULATE |
            MAP_HUGETLB, -1, 0);

    if (!rxp.rxp_db_desc[0].database_ptr)
    {
        mlnx_log("Platform Info: Allocation failed!");
        goto tidyup_context;
    }

    /* Register the memory with Mellanox */
    rxp.rxp_db_desc[0].db_umem = mlx5dv_devx_umem_reg(
                                                rxp.device_ctx,
                                                rxp.rxp_db_desc[0].database_ptr,
                                                MAX_DB_SIZE, 7);

    if (!rxp.rxp_db_desc[0].db_umem)
    {
        mlnx_log("Registration failed");
        mlnx_log("Please make sure huge pages in the system");
        mlnx_log("Hint: cat /proc/meminfo");
        mlnx_log("      echo NUM_PAGES > /proc/sys/vm/nr_hugepages");
        goto tidyup_umem0;
    }

    /*
     * Setup 2nd database memory for RXP engine 1 (RXP 1).
     * Notably both databases will be identicle. Merged remotely!
     * Setup huge pages for 2nd rule set
     */
    rxp.rxp_db_desc[1].database_ptr = mmap(NULL, MAX_DB_SIZE, PROT_READ |
            PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS | MAP_POPULATE |
            MAP_HUGETLB, -1, 0);

    if (!rxp.rxp_db_desc[1].database_ptr)
    {
        mlnx_log("Platform Info: Allocation failed!");
        goto tidyup_mmap0;
    }

    /* Register the memory with Mellanox */
    rxp.rxp_db_desc[1].db_umem = mlx5dv_devx_umem_reg(
                                                rxp.device_ctx,
                                                rxp.rxp_db_desc[1].database_ptr,
                                                MAX_DB_SIZE, 7);

    if (!rxp.rxp_db_desc[1].db_umem)
    {
        mlnx_log("Registration failed");
        mlnx_log("Please make sure huge pages in the system");
        mlnx_log("Hint: cat /proc/meminfo");
        mlnx_log("      echo NUM_PAGES > /proc/sys/vm/nr_hugepages");
        goto tidyup_umem1;
    }

    /*
     * ***********************************************************************
     * ***   Not setting up database pointer yet, wait until after programming
     * ***   Then calling mlnx_set_database()
     * ************************************************************************
     */
    err = mlx5_regex_register_read(rxp.device_ctx, 0 /*engine_id*/,
                                   RXP_CSR_IDENTIFIER, &fpga_ident);

    fpga_ident = (fpga_ident & (0x0000FFFF));
    if ((!err) || (fpga_ident != 0x5254))
    {
        mlnx_log("Error: RXP ID from RXP Eng 0 [Error:%d; FPGA ID:0x%x]",
                 err, fpga_ident);
        goto tidyup_mmap1;
    }

    mlnx_log("Info: FPGA Identifier for RXP Engine 0 - addr:0x%x:0x%x",
             RXP_CSR_IDENTIFIER, fpga_ident);

    err = mlx5_regex_register_read(rxp.device_ctx, 1 /*engine_id*/,
                                   RXP_CSR_IDENTIFIER, &fpga_ident);

    fpga_ident = (fpga_ident & (0x0000FFFF));
    if ((!err) || (fpga_ident != 0x5254))
    {
        mlnx_log("Error: RXP ID from RXP Eng 1 [Error:%d; FPGA ID:0x%x]",
                 err, fpga_ident);
        goto tidyup_mmap1;
    }

    mlnx_log("Info: FPGA Identifier for RXP Engine 1 - addr:0x%x:0x%x",
             RXP_CSR_IDENTIFIER, fpga_ident);

    return 1;

tidyup_mmap1:
    munmap(rxp.rxp_db_desc[1].database_ptr, MAX_DB_SIZE);
tidyup_umem1:
    mlx5dv_devx_umem_dereg(rxp.rxp_db_desc[1].db_umem);
tidyup_mmap0:
    munmap(rxp.rxp_db_desc[0].database_ptr, MAX_DB_SIZE);
tidyup_umem0:
    mlx5dv_devx_umem_dereg(rxp.rxp_db_desc[0].db_umem);
tidyup_context:
    //mlx5_free_context(rxp.device_ctx);//No longer in Mlnx API try:
    //free(rxp.device_ctx); //TODO check if this is right 
    //TODO Need to free device_ctx! Get updates from Yuval...
tidyup:
    ibv_free_device_list(rxp.dev_list);
    pthread_mutex_destroy(&(rxp.lock));
    //free(rxp);

    return -1;
}