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

#ifndef __MTK_CHIP_BELLWETHER_WFDMA_WA_H
#define __MTK_CHIP_BELLWETHER_WFDMA_WA_H


#include "bus/mtk_pci_dma.h"
#include "wf_wfdma_host_dma0.h"
#include "wf_wfdma_mcu_dma0.h"
#include "wf_wfdma_ext_wrap_csr.h"
#include "conn_host_csr_top.h"
#include "conn_infra_cfg.h"
#include "conn_infra_bus_cr_on.h"

/* interrupt pcie mac*/
#define MT_PCIE_MAC_BASE_ADDR   (0x10000)
#define MT_PCIE_MAC_INT_ENABLE_ADDR   (MT_PCIE_MAC_BASE_ADDR + 0x188)

#define TXD_SIZE 16
#define RXD_SIZE 16
#define RX_BUF_SIZE 1792

enum bellwether_irq_mask {
	IRQ_TX_RING16 = WF_WFDMA_HOST_DMA0_HOST_INT_STA_tx_done_int_sts_16_MASK,
	IRQ_TX_RING17 = WF_WFDMA_HOST_DMA0_HOST_INT_STA_tx_done_int_sts_17_MASK,
	IRQ_TX_RING18 = WF_WFDMA_HOST_DMA0_HOST_INT_STA_tx_done_int_sts_18_MASK,
	IRQ_TX_RING19 = WF_WFDMA_HOST_DMA0_HOST_INT_STA_tx_done_int_sts_19_MASK,
	IRQ_TX_RING20 = WF_WFDMA_HOST_DMA0_HOST_INT_STA_tx_done_int_sts_20_MASK,
	IRQ_RX_RING0 = WF_WFDMA_HOST_DMA0_HOST_INT_STA_rx_done_int_sts_0_MASK,
	IRQ_RX_RING1 = WF_WFDMA_HOST_DMA0_HOST_INT_STA_rx_done_int_sts_1_MASK,
	IRQ_RX_RING2 = WF_WFDMA_HOST_DMA0_HOST_INT_STA_rx_done_int_sts_2_MASK,
	IRQ_RX_RING3 = WF_WFDMA_HOST_DMA0_HOST_INT_STA_rx_done_int_sts_3_MASK,
	IRQ_RX_RING4 = WF_WFDMA_HOST_DMA0_HOST_INT_STA_rx_done_int_sts_4_MASK,
	IRQ_RX_RING8 = WF_WFDMA_HOST_DMA0_HOST_INT_STA_rx_done_int_sts_8_MASK,
	IRQ_LUMP_ALL = (
		IRQ_TX_RING16 | IRQ_TX_RING17 | IRQ_TX_RING18 | IRQ_TX_RING19 |
		IRQ_TX_RING20 | IRQ_RX_RING0 | IRQ_RX_RING1 | IRQ_RX_RING2 |
		IRQ_RX_RING3 | IRQ_RX_RING4 | IRQ_RX_RING8 |
		WF_WFDMA_HOST_DMA0_HOST_INT_STA_tx_coherent_int_sts_MASK |
		WF_WFDMA_HOST_DMA0_HOST_INT_STA_rx_coherent_int_sts_MASK |
		WF_WFDMA_HOST_DMA0_HOST_INT_STA_mcu2host_sw_int_sts_MASK),
};

const struct pci_tx_queue_desc txq_wa_layout[] = {
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
	{
		.cmm = {
			.hw_desc_base = WF_WFDMA_HOST_DMA0_WPDMA_TX_RING18_CTRL0_ADDR,
			.hw_int_mask = WF_WFDMA_HOST_DMA0_HOST_INT_STA_tx_done_int_sts_18_MASK,
			.q_size = 2048,
			.q_attr = Q_TX_DATA,
			.band_idx_bmp = BIT(BAND0),
			.desc_size = TXD_SIZE,
			.q_info = "band0 TXD"
		},
	},
	{
		.cmm = {
			.hw_desc_base = WF_WFDMA_HOST_DMA0_WPDMA_TX_RING19_CTRL0_ADDR,
			.hw_int_mask = WF_WFDMA_HOST_DMA0_HOST_INT_STA_tx_done_int_sts_19_MASK,
			.q_size = 2048,
			.q_attr = Q_TX_DATA,
			.band_idx_bmp = BIT(BAND2),
			.desc_size = TXD_SIZE,
			.q_info = "band2 TXD"
		},
	},
	{
		.cmm = {
			.hw_desc_base = WF_WFDMA_HOST_DMA0_WPDMA_TX_RING20_CTRL0_ADDR,
			.hw_int_mask = WF_WFDMA_HOST_DMA0_HOST_INT_STA_tx_done_int_sts_20_MASK,
			.q_size = 256,
			.q_attr = Q_TX_CMD,
			.desc_size = TXD_SIZE,
			.q_info = "cmd to WA"
		},
	},
};

const struct pci_rx_queue_desc rxq_wa_layout[] = {
	{
		.cmm = {
			.hw_desc_base = WF_WFDMA_HOST_DMA0_WPDMA_RX_RING0_CTRL0_ADDR,
			.hw_int_mask = WF_WFDMA_HOST_DMA0_HOST_INT_STA_rx_done_int_sts_0_MASK,
			.hw_didx_mask = WF_WFDMA_HOST_DMA0_WPDMA_RX_RING0_CTRL3_DMA_IDX_MASK,
			.q_size = 512,
			.q_attr = Q_RX_EVENT_WM,
			.desc_size = RXD_SIZE,
			.q_info = "event from WM",
		},
		.rx_buf_size = RX_BUF_SIZE,
	},
	{
		.cmm = {
			.hw_desc_base = WF_WFDMA_HOST_DMA0_WPDMA_RX_RING1_CTRL0_ADDR,
			.hw_int_mask = WF_WFDMA_HOST_DMA0_HOST_INT_STA_rx_done_int_sts_1_MASK,
			.hw_didx_mask = WF_WFDMA_HOST_DMA0_WPDMA_RX_RING1_CTRL3_DMA_IDX_MASK,
			.q_size = 1024,
			.q_attr = Q_RX_EVENT_WA,
			.desc_size = RXD_SIZE,
			.q_info = "event from WA",
		},
		.rx_buf_size = RX_BUF_SIZE,
	},
	{
		.cmm = {
			.hw_desc_base = WF_WFDMA_HOST_DMA0_WPDMA_RX_RING2_CTRL0_ADDR,
			.hw_int_mask = WF_WFDMA_HOST_DMA0_HOST_INT_STA_rx_done_int_sts_2_MASK,
			.hw_didx_mask = WF_WFDMA_HOST_DMA0_WPDMA_RX_RING2_CTRL3_DMA_IDX_MASK,
			.q_size = 512,
			.q_attr = Q_RX_EVENT_TX_FREE_DONE,
			.desc_size = RXD_SIZE,
			.q_info = "TX free done from WA",
		},
		.rx_buf_size = RX_BUF_SIZE,
	},
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
};

struct pci_chip_profile bellwether_pci_wa_profile = {
	.int_mask_map = {
		/* map_idx 0: Legacy interrupt bitmap */
		[0] = {
			.int_sts_addr = WF_WFDMA_HOST_DMA0_HOST_INT_STA_ADDR,
			.int_ena_addr = WF_WFDMA_HOST_DMA0_HOST_INT_ENA_ADDR,
			.int_ena_mask = IRQ_LUMP_ALL,
		},
	},
	.int_ser_reg_addr = WF_WFDMA_MCU_DMA0_HOST2MCU_SW_INT_SET_ADDR,
	.pause_trxq_reg_addr = WF_WFDMA_HOST_DMA0_WPDMA_PAUSE0_ADDR,
	.pause_trxq_mask = (
		WF_WFDMA_HOST_DMA0_WPDMA_PAUSE0_RX_RING_PAUSE_ALL_MASK |
		WF_WFDMA_HOST_DMA0_WPDMA_PAUSE0_TX_RING_PAUSE_MASK),
	.lp_fw_own_reg_addr = CONN_HOST_CSR_TOP_WF_BAND0_LPCTL_WF_B0_AP_HOST_OWNER_STATE_SYNC_ADDR,
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
	.tx_dma_glo_reg_addr = WF_WFDMA_HOST_DMA0_WPDMA_GLO_CFG_ADDR,
	.rx_dma_glo_reg_addr = WF_WFDMA_HOST_DMA0_WPDMA_GLO_CFG_ADDR,
	.txq_num = ARRAY_SIZE(txq_wa_layout),
	.rxq_num = ARRAY_SIZE(rxq_wa_layout),
	.queue_layout = {
		.tx_queue_layout = txq_wa_layout,
		.rx_queue_layout = rxq_wa_layout,
	},
	.coherent_dma_addr_size = 32,
	.non_coherent_dma_addr_size = 32,
};
#endif
