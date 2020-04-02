/*
 * rxp-csrs.h - Control + status register definitions
 *
 * Copyright 2019 Titan IC Systems
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */
#ifndef _RXP_CSRS_H_
#define _RXP_CSRS_H_

/*
 * Common to all RXP implementations
 */
#define RXP_CSR_BASE_ADDRESS		0x0000ul
#define RXP_RTRU_CSR_BASE_ADDRESS	0x0100ul
#define RXP_STATS_CSR_BASE_ADDRESS	0x0200ul
#define RXP_ROYALTY_CSR_BASE_ADDRESS	0x0600ul

#define RXP_CSR_WIDTH			4

/* This is the identifier we expect to see in the first RXP CSR */
#define RXP_IDENTIFER		0x5254

/* Hyperion specific BAR0 offsets */
#define RXP_FPGA_BASE_ADDRESS		0x0000ul
#define RXP_PCIE_BASE_ADDRESS		0x1000ul
#define RXP_IDMA_BASE_ADDRESS		0x2000ul
#define RXP_EDMA_BASE_ADDRESS		0x3000ul
#define RXP_SYSMON_BASE_ADDRESS		0xf300ul
#define RXP_ISP_CSR_BASE_ADDRESS	0xf400ul

/* Offset to the RXP common 4K CSR space */
#define RXP_PCIE_CSR_BASE_ADDRESS	0xf000ul

/* FPGA CSRs */

#define RXP_FPGA_VERSION	(RXP_FPGA_BASE_ADDRESS + RXP_CSR_WIDTH * 0)

/* PCIe CSRs */
#define RXP_PCIE_INIT_ISR	(RXP_PCIE_BASE_ADDRESS + RXP_CSR_WIDTH * 0)
#define RXP_PCIE_INIT_IMR	(RXP_PCIE_BASE_ADDRESS + RXP_CSR_WIDTH * 1)
#define RXP_PCIE_INIT_CFG_STAT	(RXP_PCIE_BASE_ADDRESS + RXP_CSR_WIDTH * 2)
#define RXP_PCIE_INIT_FLR	(RXP_PCIE_BASE_ADDRESS + RXP_CSR_WIDTH * 3)
#define RXP_PCIE_INIT_CTRL	(RXP_PCIE_BASE_ADDRESS + RXP_CSR_WIDTH * 4)

/* IDMA CSRs */
#define RXP_IDMA_ISR		(RXP_IDMA_BASE_ADDRESS + RXP_CSR_WIDTH * 0)
#define RXP_IDMA_IMR		(RXP_IDMA_BASE_ADDRESS + RXP_CSR_WIDTH * 1)
#define RXP_IDMA_CSR		(RXP_IDMA_BASE_ADDRESS + RXP_CSR_WIDTH * 4)
#define RXP_IDMA_CSR_RST_MSK	0x0001
#define RXP_IDMA_CSR_PDONE_MSK	0x0002
#define RXP_IDMA_CSR_INIT_MSK	0x0004
#define RXP_IDMA_CSR_EN_MSK	0x0008
#define RXP_IDMA_QCR		(RXP_IDMA_BASE_ADDRESS + RXP_CSR_WIDTH * 5)
#define RXP_IDMA_QCR_QAVAIL_MSK	0x00FF
#define RXP_IDMA_QCR_QEN_MSK	0xFF00
#define RXP_IDMA_DCR		(RXP_IDMA_BASE_ADDRESS + RXP_CSR_WIDTH * 6)
#define RXP_IDMA_DWCTR		(RXP_IDMA_BASE_ADDRESS + RXP_CSR_WIDTH * 7)
#define RXP_IDMA_DWTOR		(RXP_IDMA_BASE_ADDRESS + RXP_CSR_WIDTH * 8)
#define RXP_IDMA_PADCR		(RXP_IDMA_BASE_ADDRESS + RXP_CSR_WIDTH * 9)
#define RXP_IDMA_DFCR		(RXP_IDMA_BASE_ADDRESS + RXP_CSR_WIDTH * 10)
#define RXP_IDMA_FOFLR0		(RXP_IDMA_BASE_ADDRESS + RXP_CSR_WIDTH * 16)
#define RXP_IDMA_FOFLR1		(RXP_IDMA_BASE_ADDRESS + RXP_CSR_WIDTH * 17)
#define RXP_IDMA_FOFLR2		(RXP_IDMA_BASE_ADDRESS + RXP_CSR_WIDTH * 18)
#define RXP_IDMA_FUFLR0		(RXP_IDMA_BASE_ADDRESS + RXP_CSR_WIDTH * 24)
#define RXP_IDMA_FUFLR1		(RXP_IDMA_BASE_ADDRESS + RXP_CSR_WIDTH * 25)
#define RXP_IDMA_FUFLR2		(RXP_IDMA_BASE_ADDRESS + RXP_CSR_WIDTH * 26)

#define RXP_IDMA_QCSR_BASE	(RXP_IDMA_BASE_ADDRESS + RXP_CSR_WIDTH * 128)
#define RXP_IDMA_QCSR_RST_MSK	0x0001
#define RXP_IDMA_QCSR_PDONE_MSK	0x0002
#define RXP_IDMA_QCSR_INIT_MSK	0x0004
#define RXP_IDMA_QCSR_EN_MSK	0x0008
#define RXP_IDMA_QDPTR_BASE	(RXP_IDMA_BASE_ADDRESS + RXP_CSR_WIDTH * 192)
#define RXP_IDMA_QTPTR_BASE	(RXP_IDMA_BASE_ADDRESS + RXP_CSR_WIDTH * 256)
#define RXP_IDMA_QDRPTR_BASE	(RXP_IDMA_BASE_ADDRESS + RXP_CSR_WIDTH * 320)
#define RXP_IDMA_QDRALR_BASE	(RXP_IDMA_BASE_ADDRESS + RXP_CSR_WIDTH * 384)
#define RXP_IDMA_QDRAHR_BASE	(RXP_IDMA_BASE_ADDRESS + RXP_CSR_WIDTH * 385)

/* EDMA CSRs */
#define RXP_EDMA_ISR		(RXP_EDMA_BASE_ADDRESS + RXP_CSR_WIDTH * 0)
#define RXP_EDMA_IMR		(RXP_EDMA_BASE_ADDRESS + RXP_CSR_WIDTH * 1)
#define RXP_EDMA_CSR		(RXP_EDMA_BASE_ADDRESS + RXP_CSR_WIDTH * 4)
#define RXP_EDMA_CSR_RST_MSK	0x0001
#define RXP_EDMA_CSR_PDONE_MSK	0x0002
#define RXP_EDMA_CSR_INIT_MSK	0x0004
#define RXP_EDMA_CSR_EN_MSK	0x0008
#define RXP_EDMA_QCR		(RXP_EDMA_BASE_ADDRESS + RXP_CSR_WIDTH * 5)
#define RXP_EDMA_QCR_QAVAIL_MSK	0x00FF
#define RXP_EDMA_QCR_QEN_MSK	0xFF00
#define RXP_EDMA_DCR		(RXP_EDMA_BASE_ADDRESS + RXP_CSR_WIDTH * 6)
#define RXP_EDMA_DWCTR		(RXP_EDMA_BASE_ADDRESS + RXP_CSR_WIDTH * 7)
#define RXP_EDMA_DWTOR		(RXP_EDMA_BASE_ADDRESS + RXP_CSR_WIDTH * 8)
#define RXP_EDMA_DFCR		(RXP_EDMA_BASE_ADDRESS + RXP_CSR_WIDTH * 10)
#define RXP_EDMA_FOFLR0		(RXP_EDMA_BASE_ADDRESS + RXP_CSR_WIDTH * 16)
#define RXP_EDMA_FOFLR1		(RXP_EDMA_BASE_ADDRESS + RXP_CSR_WIDTH * 17)
#define RXP_EDMA_FOFLR2		(RXP_EDMA_BASE_ADDRESS + RXP_CSR_WIDTH * 18)
#define RXP_EDMA_FUFLR0		(RXP_EDMA_BASE_ADDRESS + RXP_CSR_WIDTH * 24)
#define RXP_EDMA_FUFLR1		(RXP_EDMA_BASE_ADDRESS + RXP_CSR_WIDTH * 25)
#define RXP_EDMA_FUFLR2		(RXP_EDMA_BASE_ADDRESS + RXP_CSR_WIDTH * 26)

#define RXP_EDMA_QCSR_BASE	(RXP_EDMA_BASE_ADDRESS + RXP_CSR_WIDTH * 128)
#define RXP_EDMA_QCSR_RST_MSK	0x0001
#define RXP_EDMA_QCSR_PDONE_MSK	0x0002
#define RXP_EDMA_QCSR_INIT_MSK	0x0004
#define RXP_EDMA_QCSR_EN_MSK	0x0008
#define RXP_EDMA_QTPTR_BASE	(RXP_EDMA_BASE_ADDRESS + RXP_CSR_WIDTH * 256)
#define RXP_EDMA_QDRPTR_BASE	(RXP_EDMA_BASE_ADDRESS + RXP_CSR_WIDTH * 320)
#define RXP_EDMA_QDRALR_BASE	(RXP_EDMA_BASE_ADDRESS + RXP_CSR_WIDTH * 384)
#define RXP_EDMA_QDRAHR_BASE	(RXP_EDMA_BASE_ADDRESS + RXP_CSR_WIDTH * 385)

/* Main CSRs */
#define RXP_CSR_IDENTIFIER	(RXP_CSR_BASE_ADDRESS + RXP_CSR_WIDTH * 0)
#define RXP_CSR_REVISION	(RXP_CSR_BASE_ADDRESS + RXP_CSR_WIDTH * 1)
#define RXP_CSR_CAPABILITY_0	(RXP_CSR_BASE_ADDRESS + RXP_CSR_WIDTH * 2)
#define RXP_CSR_CAPABILITY_1	(RXP_CSR_BASE_ADDRESS + RXP_CSR_WIDTH * 3)
#define RXP_CSR_CAPABILITY_2	(RXP_CSR_BASE_ADDRESS + RXP_CSR_WIDTH * 4)
#define RXP_CSR_CAPABILITY_3	(RXP_CSR_BASE_ADDRESS + RXP_CSR_WIDTH * 5)
#define RXP_CSR_CAPABILITY_4	(RXP_CSR_BASE_ADDRESS + RXP_CSR_WIDTH * 6)
#define RXP_CSR_CAPABILITY_5	(RXP_CSR_BASE_ADDRESS + RXP_CSR_WIDTH * 7)
#define RXP_CSR_CAPABILITY_6	(RXP_CSR_BASE_ADDRESS + RXP_CSR_WIDTH * 8)
#define RXP_CSR_CAPABILITY_7	(RXP_CSR_BASE_ADDRESS + RXP_CSR_WIDTH * 9)
#define RXP_CSR_STATUS		(RXP_CSR_BASE_ADDRESS + RXP_CSR_WIDTH * 10)
#define RXP_CSR_STATUS_INIT_DONE	0x0001
#define RXP_CSR_STATUS_GOING		0x0008
#define RXP_CSR_STATUS_IDLE		0x0040
#define RXP_CSR_STATUS_TRACKER_OK	0x0080
#define RXP_CSR_STATUS_TRIAL_TIMEOUT	0x0100
#define RXP_CSR_FIFO_STATUS_0	(RXP_CSR_BASE_ADDRESS + RXP_CSR_WIDTH * 11)
#define RXP_CSR_FIFO_STATUS_1	(RXP_CSR_BASE_ADDRESS + RXP_CSR_WIDTH * 12)
#define RXP_CSR_JOB_DDOS_COUNT	(RXP_CSR_BASE_ADDRESS + RXP_CSR_WIDTH * 13)
/* 14 + 15 reserved */
#define RXP_CSR_CORE_CLK_COUNT	(RXP_CSR_BASE_ADDRESS + RXP_CSR_WIDTH * 16)
#define RXP_CSR_WRITE_COUNT	(RXP_CSR_BASE_ADDRESS + RXP_CSR_WIDTH * 17)
#define RXP_CSR_JOB_COUNT	(RXP_CSR_BASE_ADDRESS + RXP_CSR_WIDTH * 18)
#define RXP_CSR_JOB_ERROR_COUNT	(RXP_CSR_BASE_ADDRESS + RXP_CSR_WIDTH * 19)
#define RXP_CSR_JOB_BYTE_COUNT0	(RXP_CSR_BASE_ADDRESS + RXP_CSR_WIDTH * 20)
#define RXP_CSR_JOB_BYTE_COUNT1	(RXP_CSR_BASE_ADDRESS + RXP_CSR_WIDTH * 21)
#define RXP_CSR_RESPONSE_COUNT	(RXP_CSR_BASE_ADDRESS + RXP_CSR_WIDTH * 22)
#define RXP_CSR_MATCH_COUNT	(RXP_CSR_BASE_ADDRESS + RXP_CSR_WIDTH * 23)
#define RXP_CSR_CTRL		(RXP_CSR_BASE_ADDRESS + RXP_CSR_WIDTH * 24)
#define RXP_CSR_CTRL_INIT	0x0001
#define RXP_CSR_CTRL_GO		0x0008
#define RXP_CSR_MAX_MATCH	(RXP_CSR_BASE_ADDRESS + RXP_CSR_WIDTH * 25)
#define RXP_CSR_MAX_PREFIX	(RXP_CSR_BASE_ADDRESS + RXP_CSR_WIDTH * 26)
#define RXP_CSR_MAX_PRI_THREAD	(RXP_CSR_BASE_ADDRESS + RXP_CSR_WIDTH * 27)
#define RXP_CSR_MAX_LATENCY	(RXP_CSR_BASE_ADDRESS + RXP_CSR_WIDTH * 28)
#define RXP_CSR_SCRATCH_1	(RXP_CSR_BASE_ADDRESS + RXP_CSR_WIDTH * 29)
#define RXP_CSR_CLUSTER_MASK	(RXP_CSR_BASE_ADDRESS + RXP_CSR_WIDTH * 30)
#define RXP_CSR_INTRA_CLUSTER_MASK (RXP_CSR_BASE_ADDRESS + RXP_CSR_WIDTH * 31)

/* Runtime Rule Update CSRs */
/* 0 + 1 reserved */
#define RXP_RTRU_CSR_CAPABILITY	(RXP_RTRU_CSR_BASE_ADDRESS + RXP_CSR_WIDTH * 2)
/* 3-9 reserved */
#define RXP_RTRU_CSR_STATUS	(RXP_RTRU_CSR_BASE_ADDRESS + RXP_CSR_WIDTH * 10)
#define RXP_RTRU_CSR_STATUS_UPDATE_DONE		0x0002
#define RXP_RTRU_CSR_STATUS_IM_INIT_DONE	0x0010
#define RXP_RTRU_CSR_STATUS_L1C_INIT_DONE	0x0020
#define RXP_RTRU_CSR_STATUS_L2C_INIT_DONE	0x0040
#define RXP_RTRU_CSR_STATUS_EM_INIT_DONE	0x0080
#define RXP_RTRU_CSR_FIFO_STAT	(RXP_RTRU_CSR_BASE_ADDRESS + RXP_CSR_WIDTH * 11)
/* 12-15 reserved */
#define RXP_RTRU_CSR_CHECKSUM_0	(RXP_RTRU_CSR_BASE_ADDRESS + RXP_CSR_WIDTH * 16)
#define RXP_RTRU_CSR_CHECKSUM_1	(RXP_RTRU_CSR_BASE_ADDRESS + RXP_CSR_WIDTH * 17)
#define RXP_RTRU_CSR_CHECKSUM_2	(RXP_RTRU_CSR_BASE_ADDRESS + RXP_CSR_WIDTH * 18)
/* 19 + 20 reserved */
#define RXP_RTRU_CSR_RTRU_COUNT	(RXP_RTRU_CSR_BASE_ADDRESS + RXP_CSR_WIDTH * 21)
#define RXP_RTRU_CSR_ROF_REV	(RXP_RTRU_CSR_BASE_ADDRESS + RXP_CSR_WIDTH * 22)
/* 23 reserved */
#define RXP_RTRU_CSR_CTRL	(RXP_RTRU_CSR_BASE_ADDRESS + RXP_CSR_WIDTH * 24)
#define RXP_RTRU_CSR_CTRL_INIT		0x0001
#define RXP_RTRU_CSR_CTRL_GO		0x0002
#define RXP_RTRU_CSR_CTRL_SIP		0x0004
#define RXP_RTRU_CSR_CTRL_INIT_MODE_MASK		(3 << 4)
#define RXP_RTRU_CSR_CTRL_INIT_MODE_IM_L1_L2_EM	(0 << 4)
#define RXP_RTRU_CSR_CTRL_INIT_MODE_IM_L1_L2	(1 << 4)
#define RXP_RTRU_CSR_CTRL_INIT_MODE_L1_L2		(2 << 4)
#define RXP_RTRU_CSR_CTRL_INIT_MODE_EM			(3 << 4)
#define RXP_RTRU_CSR_ADDR	(RXP_RTRU_CSR_BASE_ADDRESS + RXP_CSR_WIDTH * 25)
#define RXP_RTRU_CSR_DATA_0	(RXP_RTRU_CSR_BASE_ADDRESS + RXP_CSR_WIDTH * 26)
#define RXP_RTRU_CSR_DATA_1	(RXP_RTRU_CSR_BASE_ADDRESS + RXP_CSR_WIDTH * 27)
/* 28-31 reserved */

/* Statistics CSRs */
#define RXP_STATS_CSR_CLUSTER	(RXP_STATS_CSR_BASE_ADDRESS + RXP_CSR_WIDTH * 0)
#define RXP_STATS_CSR_L2_CACHE	(RXP_STATS_CSR_BASE_ADDRESS + RXP_CSR_WIDTH * 24)
#define RXP_STATS_CSR_MPFE_FIFO	(RXP_STATS_CSR_BASE_ADDRESS + RXP_CSR_WIDTH * 25)
#define RXP_STATS_CSR_PE	(RXP_STATS_CSR_BASE_ADDRESS + RXP_CSR_WIDTH * 28)
#define RXP_STATS_CSR_CP	(RXP_STATS_CSR_BASE_ADDRESS + RXP_CSR_WIDTH * 30)
#define RXP_STATS_CSR_DP	(RXP_STATS_CSR_BASE_ADDRESS + RXP_CSR_WIDTH * 31)

/* Sysmon Stats CSRs */
#define RXP_SYSMON_CSR_T_FPGA	(RXP_SYSMON_BASE_ADDRESS + RXP_CSR_WIDTH * 0)
#define RXP_SYSMON_CSR_V_VCCINT	(RXP_SYSMON_BASE_ADDRESS + RXP_CSR_WIDTH * 1)
#define RXP_SYSMON_CSR_V_VCCAUX	(RXP_SYSMON_BASE_ADDRESS + RXP_CSR_WIDTH * 2)
#define RXP_SYSMON_CSR_T_U1	(RXP_SYSMON_BASE_ADDRESS + RXP_CSR_WIDTH * 20)
#define RXP_SYSMON_CSR_I_EDG12V	(RXP_SYSMON_BASE_ADDRESS + RXP_CSR_WIDTH * 21)
#define RXP_SYSMON_CSR_I_VCC3V3	(RXP_SYSMON_BASE_ADDRESS + RXP_CSR_WIDTH * 22)
#define RXP_SYSMON_CSR_I_VCC2V5	(RXP_SYSMON_BASE_ADDRESS + RXP_CSR_WIDTH * 23)
#define RXP_SYSMON_CSR_T_U2	(RXP_SYSMON_BASE_ADDRESS + RXP_CSR_WIDTH * 28)
#define RXP_SYSMON_CSR_I_AUX12V	(RXP_SYSMON_BASE_ADDRESS + RXP_CSR_WIDTH * 29)
#define RXP_SYSMON_CSR_I_VCC1V8	(RXP_SYSMON_BASE_ADDRESS + RXP_CSR_WIDTH * 30)
#define RXP_SYSMON_CSR_I_VDDR3	(RXP_SYSMON_BASE_ADDRESS + RXP_CSR_WIDTH * 31)

/* In Service Programming CSRs */

/* RXP-F1 and RXP-ZYNQ specific CSRs */
#define MQ_CP_BASE					(0x0500ul)
#define MQ_CP_CAPABILITY_BASE		(MQ_CP_BASE + 2 * RXP_CSR_WIDTH)
#define MQ_CP_CAPABILITY_0			(MQ_CP_CAPABILITY_BASE + 0 * RXP_CSR_WIDTH)
#define MQ_CP_CAPABILITY_1			(MQ_CP_CAPABILITY_BASE + 1 * RXP_CSR_WIDTH)
#define MQ_CP_CAPABILITY_2			(MQ_CP_CAPABILITY_BASE + 2 * RXP_CSR_WIDTH)
#define MQ_CP_CAPABILITY_3			(MQ_CP_CAPABILITY_BASE + 3 * RXP_CSR_WIDTH)
#define MQ_CP_FIFO_STATUS_BASE		(MQ_CP_BASE + 11 * RXP_CSR_WIDTH)
#define MQ_CP_FIFO_STATUS_C0		(MQ_CP_FIFO_STATUS_BASE + 0 * RXP_CSR_WIDTH)
#define MQ_CP_FIFO_STATUS_C1		(MQ_CP_FIFO_STATUS_BASE + 1 * RXP_CSR_WIDTH)
#define MQ_CP_FIFO_STATUS_C2		(MQ_CP_FIFO_STATUS_BASE + 2 * RXP_CSR_WIDTH)
#define MQ_CP_FIFO_STATUS_C3		(MQ_CP_FIFO_STATUS_BASE + 3 * RXP_CSR_WIDTH)

/* Royalty tracker / licensing related CSRs */
#define RXPL_CSR_IDENT		(RXP_ROYALTY_CSR_BASE_ADDRESS + 0 * RXP_CSR_WIDTH)
#define RXPL_IDENTIFIER		0x4c505852			/* RXPL */
#define RXPL_CSR_CAPABILITY	(RXP_ROYALTY_CSR_BASE_ADDRESS + 2 * RXP_CSR_WIDTH)
#define RXPL_TYPE_MASK		0xFF
#define RXPL_TYPE_NONE		0
#define RXPL_TYPE_MAXIM		1
#define RXPL_TYPE_XILINX_DNA	2
#define RXPL_CSR_STATUS		(RXP_ROYALTY_CSR_BASE_ADDRESS + 10 * RXP_CSR_WIDTH)
#define RXPL_CSR_IDENT_0	(RXP_ROYALTY_CSR_BASE_ADDRESS + 16 * RXP_CSR_WIDTH)
#define RXPL_CSR_KEY_0		(RXP_ROYALTY_CSR_BASE_ADDRESS + 24 * RXP_CSR_WIDTH)

#endif /* _RXP_CSRS_H_ */