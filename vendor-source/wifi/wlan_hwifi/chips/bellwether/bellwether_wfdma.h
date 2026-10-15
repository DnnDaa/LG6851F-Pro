/*
 * Copyright (c) [2020] MediaTek Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 * list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its contributors
 * may be used to endorse or promote products derived from this software without
 * specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ""AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef __MTK_CHIP_BELLWETHER_WFDMA_H
#define __MTK_CHIP_BELLWETHER_WFDMA_H


#include "bus/mtk_pci_dma.h"
#include "wf_wfdma_host_dma0.h"
#include "wf_wfdma_mcu_dma0.h"
#include "wf_wfdma_ext_wrap_csr.h"
#include "conn_host_csr_top.h"
#include "conn_infra_cfg.h"
#include "conn_infra_bus_cr_on.h"
#include "wf_hif_dmashdl_top.h"

/* interrupt pcie mac*/
#define MT_PCIE_MAC_BASE_ADDR   (0x10000)
#define MT_PCIE_MAC_INT_ENABLE_ADDR   (MT_PCIE_MAC_BASE_ADDR + 0x188)


#define TXD_SIZE 16
#define RXD_SIZE 16
#define RX_BUF_SIZE 1792

const struct pci_tx_queue_desc txq_layout[] = {
	{
		.cmm = {
			.hw_desc_base = WF_WFDMA_HOST_DMA0_WPDMA_TX_RING0_CTRL0_ADDR,
			.hw_int_mask = WF_WFDMA_HOST_DMA0_HOST_INT_STA_tx_done_int_sts_0_MASK,
			.q_size = 2048,
			.q_attr = Q_TX_DATA,
			.band_idx_bmp = BIT(BAND0),
			.desc_size = TXD_SIZE,
			.q_info = "band0 TXD"
		},
	},
	{
		.cmm = {
			.hw_desc_base = WF_WFDMA_HOST_DMA0_WPDMA_TX_RING1_CTRL0_ADDR,
			.hw_int_mask = WF_WFDMA_HOST_DMA0_HOST_INT_STA_tx_done_int_sts_1_MASK,
			.q_size = 2048,
			.q_attr = Q_TX_DATA,
			.desc_size = TXD_SIZE,
			.band_idx_bmp = BIT(BAND1),
			.q_info = "band1 TXD"
		},
	},
	{
		.cmm = {
			.hw_desc_base = WF_WFDMA_HOST_DMA0_WPDMA_TX_RING2_CTRL0_ADDR,
			.hw_int_mask = WF_WFDMA_HOST_DMA0_HOST_INT_STA_tx_done_int_sts_2_MASK,
			.q_size = 2048,
			.q_attr = Q_TX_DATA,
			.desc_size = TXD_SIZE,
			.band_idx_bmp = BIT(BAND2),
			.q_info = "band2 TXD"
		},
	},
	{
		.cmm = {
			.hw_desc_base = WF_WFDMA_HOST_DMA0_WPDMA_TX_RING16_CTRL0_ADDR,
			.hw_int_mask = WF_WFDMA_HOST_DMA0_HOST_INT_STA_tx_done_int_sts_16_MASK,
			.q_size = 256,
			.q_attr = Q_TX_FWDL,
			.desc_size = TXD_SIZE,
			.q_info = "FWDL"
		},
	},
	{
		.cmm = {
			.hw_desc_base = WF_WFDMA_HOST_DMA0_WPDMA_TX_RING17_CTRL0_ADDR,
			.hw_int_mask = WF_WFDMA_HOST_DMA0_HOST_INT_STA_tx_done_int_sts_17_MASK,
			.q_size = 256,
			.q_attr = Q_TX_CMD_WM,
			.desc_size = TXD_SIZE,
			.q_info = "cmd to WM"
		},
	},
};

const struct pci_rx_queue_desc rxq_layout[] = {
	{
		.cmm = {
			.hw_desc_base = WF_WFDMA_HOST_DMA0_WPDMA_RX_RING4_CTRL0_ADDR,
			.hw_int_mask = WF_WFDMA_HOST_DMA0_HOST_INT_STA_rx_done_int_sts_4_MASK,
			.hw_didx_mask = WF_WFDMA_HOST_DMA0_WPDMA_RX_RING4_CTRL3_DMA_IDX_MASK,
			.q_size = 1536,
			.q_attr = Q_RX_DATA,
			.desc_size = RXD_SIZE,
			.band_idx_bmp = BIT(BAND0),
			.q_info = "band0 RX data"
		},
		.rx_buf_size = RX_BUF_SIZE,
	},
	{
		.cmm = {
			.hw_desc_base = WF_WFDMA_HOST_DMA0_WPDMA_RX_RING5_CTRL0_ADDR,
			.hw_int_mask = WF_WFDMA_HOST_DMA0_HOST_INT_STA_rx_done_int_sts_5_MASK,
			.hw_didx_mask = WF_WFDMA_HOST_DMA0_WPDMA_RX_RING5_CTRL3_DMA_IDX_MASK,
			.q_size = 1536,
			.q_attr = Q_RX_DATA,
			.desc_size = RXD_SIZE,
			.band_idx_bmp = BIT(BAND1),
			.q_info = "band1 RX data"
		},
		.rx_buf_size = RX_BUF_SIZE,
	},
	{
		.cmm = {
			.hw_desc_base = WF_WFDMA_HOST_DMA0_WPDMA_RX_RING8_CTRL0_ADDR,
			.hw_int_mask = WF_WFDMA_HOST_DMA0_HOST_INT_STA_rx_done_int_sts_8_MASK,
			.hw_didx_mask = WF_WFDMA_HOST_DMA0_WPDMA_RX_RING8_CTRL3_DMA_IDX_MASK,
			.q_size = 1536,
			.q_attr = Q_RX_DATA,
			.desc_size = RXD_SIZE,
			.band_idx_bmp = BIT(BAND2),
			.q_info = "band2 RX data"
		},
		.rx_buf_size = RX_BUF_SIZE,
	},
	{
		.cmm = {
			.hw_desc_base = WF_WFDMA_HOST_DMA0_WPDMA_RX_RING0_CTRL0_ADDR,
			.hw_int_mask = WF_WFDMA_HOST_DMA0_HOST_INT_STA_rx_done_int_sts_0_MASK,
			.hw_didx_mask = WF_WFDMA_HOST_DMA0_WPDMA_RX_RING0_CTRL3_DMA_IDX_MASK,
			.q_size = 512,
			.q_attr = Q_RX_EVENT_WM,
			.desc_size = RXD_SIZE,
			.q_info = "event from WM"
		},
		.rx_buf_size = RX_BUF_SIZE,
	},
	{
		.cmm = {
			.hw_desc_base = WF_WFDMA_HOST_DMA0_WPDMA_RX_RING6_CTRL0_ADDR,
			.hw_int_mask = WF_WFDMA_HOST_DMA0_HOST_INT_STA_rx_done_int_sts_6_MASK,
			.hw_didx_mask = WF_WFDMA_HOST_DMA0_WPDMA_RX_RING6_CTRL3_DMA_IDX_MASK,
			.q_size = 1024,
			.q_attr = Q_RX_EVENT_WM,
			.desc_size = RXD_SIZE,
			.q_info = "band0 tx free done",
		},
		.rx_buf_size = RX_BUF_SIZE,
	},
	{
		.cmm = {
			.hw_desc_base = WF_WFDMA_HOST_DMA0_WPDMA_RX_RING7_CTRL0_ADDR,
			.hw_int_mask = WF_WFDMA_HOST_DMA0_HOST_INT_STA_rx_done_int_sts_7_MASK,
			.hw_didx_mask = WF_WFDMA_HOST_DMA0_WPDMA_RX_RING7_CTRL3_DMA_IDX_MASK,
			.q_size = 1024,
			.q_attr = Q_RX_EVENT_WM,
			.desc_size = RXD_SIZE,
			.q_info = "band1 tx free done",
		},
		.rx_buf_size = RX_BUF_SIZE,
	},
	{
		.cmm = {
			.hw_desc_base = WF_WFDMA_HOST_DMA0_WPDMA_RX_RING9_CTRL0_ADDR,
			.hw_int_mask = WF_WFDMA_HOST_DMA0_HOST_INT_STA_rx_done_int_sts_9_MASK,
			.hw_didx_mask = WF_WFDMA_HOST_DMA0_WPDMA_RX_RING9_CTRL3_DMA_IDX_MASK,
			.q_size = 1024,
			.q_attr = Q_RX_EVENT_WM,
			.desc_size = RXD_SIZE,
			.q_info = "band2 tx free done",
		},
		.rx_buf_size = RX_BUF_SIZE,
	},
};

struct pci_chip_profile bellwether_pci_profile = {
	.int_mask_map = {
		/* map_idx 0: Legacy interrupt bitmap */
		[0] = {
			.int_sts_addr = WF_WFDMA_HOST_DMA0_HOST_INT_STA_ADDR,
			.int_ena_addr = WF_WFDMA_HOST_DMA0_HOST_INT_ENA_ADDR,
			.int_ena_mask = (
				WF_WFDMA_HOST_DMA0_HOST_INT_STA_tx_done_int_sts_0_MASK |
				WF_WFDMA_HOST_DMA0_HOST_INT_STA_tx_done_int_sts_1_MASK |
				WF_WFDMA_HOST_DMA0_HOST_INT_STA_tx_done_int_sts_2_MASK |
				WF_WFDMA_HOST_DMA0_HOST_INT_STA_tx_done_int_sts_16_MASK |
				WF_WFDMA_HOST_DMA0_HOST_INT_STA_tx_done_int_sts_17_MASK |
				WF_WFDMA_HOST_DMA0_HOST_INT_STA_rx_done_int_sts_0_MASK |
				WF_WFDMA_HOST_DMA0_HOST_INT_STA_rx_done_int_sts_4_MASK |
				WF_WFDMA_HOST_DMA0_HOST_INT_STA_rx_done_int_sts_5_MASK |
				WF_WFDMA_HOST_DMA0_HOST_INT_STA_rx_done_int_sts_6_MASK |
				WF_WFDMA_HOST_DMA0_HOST_INT_STA_rx_done_int_sts_7_MASK |
				WF_WFDMA_HOST_DMA0_HOST_INT_STA_rx_done_int_sts_8_MASK |
				WF_WFDMA_HOST_DMA0_HOST_INT_STA_rx_done_int_sts_9_MASK |
				WF_WFDMA_HOST_DMA0_HOST_INT_STA_tx_coherent_int_sts_MASK |
				WF_WFDMA_HOST_DMA0_HOST_INT_STA_rx_coherent_int_sts_MASK |
				WF_WFDMA_HOST_DMA0_HOST_INT_STA_subsys_int_sts_MASK |
				WF_WFDMA_HOST_DMA0_HOST_INT_STA_mcu2host_sw_int_sts_MASK)
		},
	},
	.int_ser_reg_addr = WF_WFDMA_MCU_DMA0_HOST2MCU_SW_INT_SET_ADDR,
	.pause_trxq_reg_addr = WF_WFDMA_HOST_DMA0_WPDMA_PAUSE0_ADDR,
	.pause_trxq_mask = (
	WF_WFDMA_HOST_DMA0_WPDMA_PAUSE0_RX_RING_PAUSE_ALL_MASK |
	WF_WFDMA_HOST_DMA0_WPDMA_PAUSE0_TX_RING_PAUSE_MASK),
	.lp_fw_own_reg_addr = CONN_HOST_CSR_TOP_WF_BAND0_LPCTL_WF_B0_AP_HOST_OWNER_STATE_SYNC_ADDR,
	.txq_num = ARRAY_SIZE(txq_layout),
	.rxq_num = ARRAY_SIZE(rxq_layout),
	.queue_layout = {
		.tx_queue_layout = txq_layout,
		.rx_queue_layout = rxq_layout,
	},
	/*mcu2host swi*/
	.int_enable_swi_mask =
		WF_WFDMA_HOST_DMA0_HOST_INT_ENA_mcu2host_sw_int_ena_MASK,
	.mcu2host_sw_int_ena_addr = WF_WFDMA_HOST_DMA0_MCU2HOST_SW_INT_ENA_ADDR,
	.mcu2host_sw_int_sta_addr = WF_WFDMA_HOST_DMA0_MCU2HOST_SW_INT_STA_ADDR,
	.mcu2host_sw_int_ser_mask = (
	WF_WFDMA_HOST_DMA0_MCU2HOST_SW_INT_STA_mcu2host_sw_int_2_MASK |
	WF_WFDMA_HOST_DMA0_MCU2HOST_SW_INT_STA_mcu2host_sw_int_3_MASK |
	WF_WFDMA_HOST_DMA0_MCU2HOST_SW_INT_STA_mcu2host_sw_int_4_MASK |
	WF_WFDMA_HOST_DMA0_MCU2HOST_SW_INT_STA_mcu2host_sw_int_5_MASK),
	.mcu2host_sw_wdt_ser_mask = (
	WF_WFDMA_HOST_DMA0_MCU2HOST_SW_INT_STA_mcu2host_wacpu_wdt_MASK |
	WF_WFDMA_HOST_DMA0_MCU2HOST_SW_INT_STA_mcu2host_wmcpu_wdt_MASK),
};

enum wfdma_schdl_group_index {
	ENUM_DMASHDL_GROUP_0 = 0,
	ENUM_DMASHDL_GROUP_1,
	ENUM_DMASHDL_GROUP_2,
	ENUM_DMASHDL_GROUP_3,
	ENUM_DMASHDL_GROUP_4,
	ENUM_DMASHDL_GROUP_5,
	ENUM_DMASHDL_GROUP_6,
	ENUM_DMASHDL_GROUP_7,
	ENUM_DMASHDL_GROUP_8,
	ENUM_DMASHDL_GROUP_9,
	ENUM_DMASHDL_GROUP_10,
	ENUM_DMASHDL_GROUP_11,
	ENUM_DMASHDL_GROUP_12,
	ENUM_DMASHDL_GROUP_13,
	ENUM_DMASHDL_GROUP_14,
	ENUM_DMASHDL_GROUP_15,
	ENUM_DMASHDL_GROUP_NUM
};

struct wfdma_schdl_cfg {
	bool slot_arbiter_enable;
	u16 ple_max_page;
	u16 pse_max_page;
	bool refill_enable[ENUM_DMASHDL_GROUP_NUM];
	u16 max_quota[ENUM_DMASHDL_GROUP_NUM];
	u16 min_quota[ENUM_DMASHDL_GROUP_NUM];
	u8 que_grp_mapping[ENUM_DMASHDL_GROUP_NUM * 2];
	u8 pri_grp_mapping[ENUM_DMASHDL_GROUP_NUM];
};

struct wfdma_schdl_cfg dma_schdl_cfg = {
	.slot_arbiter_enable = false,

	.ple_max_page = 0x1,

	.pse_max_page = 0x0,

	.refill_enable = {
		true,
		true,
		true,
		false,
		false,
		false,
		false,
		false,
		false,
		false,
		false,
		false,
		false,
		false,
		false,
		false,
	},

	.max_quota = {
		0xFFF,
		0xFFF,
		0xFFF,
		0x0,
		0x0,
		0x0,
		0x0,
		0x0,
		0x0,
		0x0,
		0x0,
		0x0,
		0x0,
		0x0,
		0x0,
		0x0,
	},

	.min_quota = {
		0x10,
		0x10,
		0x10,
		0x0,
		0x0,
		0x0,
		0x0,
		0x0,
		0x0,
		0x0,
		0x0,
		0x0,
		0x0,
		0x0,
		0x0,
		0x0,
	},

	.que_grp_mapping = {
		0x0, /* LMAC AC00 */
		0x0, /* LMAC AC01 */
		0x0, /* LMAC AC02 */
		0x0, /* LMAC AC03 */
		0x1, /* LMAC AC10 */
		0x1, /* LMAC AC11 */
		0x1, /* LMAC AC12 */
		0x1, /* LMAC AC13 */
		0x2, /* LMAC AC20 */
		0x2, /* LMAC AC21 */
		0x2, /* LMAC AC22 */
		0x2, /* LMAC AC23 */
		0x0, /* LMAC AC30 */
		0x0, /* LMAC AC31 */
		0x0, /* LMAC AC32 */
		0x0, /* LMAC AC33 */
		0x0, /* ALTX */
		0x0, /* BMC */
		0x0, /* BCN */
		0x1, /* HW Reserved */
		0x1, /* HW Reserved */
		0x1, /* HW Reserved */
		0x1, /* HW Reserved */
		0x1, /* HW Reserved */
		0x0, /* NAF */
		0x0, /* NBCN */
		0x0, /* FIXFID */
		0x1, /* HW Reserved */
		0x1, /* HW Reserved */
		0x1, /* HW Reserved */
		0x1, /* HW Reserved */
		0x1, /* HW Reserved */
	},

	.pri_grp_mapping = {
		0x0,
		0x1,
		0x2,
		0x3,
		0x4,
		0x5,
		0x6,
		0x7,
		0x8,
		0x9,
		0xA,
		0xB,
		0xC,
		0xD,
		0xE,
		0xF,
	},
};


#endif
