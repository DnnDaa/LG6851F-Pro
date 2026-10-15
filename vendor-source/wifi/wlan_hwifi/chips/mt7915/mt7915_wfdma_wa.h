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

#ifndef __MTK_CHIP_MT7915_PDMA_H
#define __MTK_CHIP_MT7915_PDMA_H


#include "bus/mtk_pci_dma.h"
#include "wf_wfdma_host_dma0.h"
#include "wf_wfdma_host_dma1.h"
#include "wf_wfdma_host_dma0_pcie1.h"
#include "wf_wfdma_host_dma1_pcie1.h"
#include "wf_wfdma_mcu_dma0.h"
#include "wf_wfdma_mcu_dma1.h"
#include "wf_wfdma_mcu_dma1_pcie1.h"
#include "wf_wfdma_mem_dma.h"
#include "wf_wfdma_ext_wrap_csr.h"
#include "conn_host_csr_top.h"
#include "conn_infra_cfg.h"


/* interrupt pcie 0 */
#define MT_PCIE_MAC_BASE_ADDR   (0x74030000)
#define MT_PCIE_MAC_INT_ENABLE_ADDR   (MT_PCIE_MAC_BASE_ADDR + 0x188)

/* interrupt pcie 1 */
#define MT_PCIE_MAC1_BASE_ADDR   (0x74020000)
#define MT_PCIE_MAC1_INT_ENABLE_ADDR   (MT_PCIE_MAC1_BASE_ADDR + 0x188)

#define TXD_SIZE 16
#define RXD_SIZE 16
#define RX_BUF_SIZE 1600

enum mt7915_irq_mask {
	IRQ_DMA1_TX_RING16 = WF_WFDMA_EXT_WRAP_CSR_WFDMA_HOST_INT_STA_host_dma1_tx_done_int_sts_16_MASK,
	IRQ_DMA1_TX_RING17 = WF_WFDMA_EXT_WRAP_CSR_WFDMA_HOST_INT_STA_host_dma1_tx_done_int_sts_17_MASK,
	IRQ_DMA1_TX_RING18 = WF_WFDMA_EXT_WRAP_CSR_WFDMA_HOST_INT_STA_host_dma1_tx_done_int_sts_18_MASK,
	IRQ_DMA1_TX_RING19 = WF_WFDMA_EXT_WRAP_CSR_WFDMA_HOST_INT_STA_host_dma1_tx_done_int_sts_19_MASK,
	IRQ_DMA1_TX_RING20 = WF_WFDMA_EXT_WRAP_CSR_WFDMA_HOST_INT_STA_host_dma1_tx_done_int_sts_20_MASK,
	IRQ_DMA0_RX_RING0 = WF_WFDMA_EXT_WRAP_CSR_WFDMA_HOST_INT_STA_host_dma0_rx_don_int_sts_0_MASK,
	IRQ_DMA0_RX_RING1 = WF_WFDMA_EXT_WRAP_CSR_WFDMA_HOST_INT_STA_host_dma0_rx_don_int_sts_1_MASK,
	IRQ_DMA1_RX_RING0 = WF_WFDMA_EXT_WRAP_CSR_WFDMA_HOST_INT_STA_host_dma1_rx_done_int_sts_0_MASK,
	IRQ_DMA1_RX_RING1 = WF_WFDMA_EXT_WRAP_CSR_WFDMA_HOST_INT_STA_host_dma1_rx_done_int_sts_1_MASK,
	IRQ_DMA1_RX_RING2 = WF_WFDMA_EXT_WRAP_CSR_WFDMA_HOST_INT_STA_host_dma1_rx_done_int_sts_2_MASK,
	IRQ_DMA1_TX_RING0_WED = WF_WFDMA_EXT_WRAP_CSR_WED_HOST_INT_STA_host_dma1_tx_done_int_sts_0_MASK,
	IRQ_DMA1_TX_RING1_WED = WF_WFDMA_EXT_WRAP_CSR_WED_HOST_INT_STA_host_dma1_tx_done_int_sts_1_MASK,
	IRQ_DMA1_TX_RING16_WED = WF_WFDMA_EXT_WRAP_CSR_WED_HOST_INT_STA_host_dma1_tx_done_int_sts_16_MASK,
	IRQ_DMA1_TX_RING17_WED = WF_WFDMA_EXT_WRAP_CSR_WED_HOST_INT_STA_host_dma1_tx_done_int_sts_17_MASK,
	IRQ_DMA1_TX_RING20_WED = WF_WFDMA_EXT_WRAP_CSR_WED_HOST_INT_STA_host_dma1_tx_done_int_sts_20_MASK,
	IRQ_DMA0_RX_RING0_WED = WF_WFDMA_EXT_WRAP_CSR_WED_HOST_INT_STA_host_dma0_rx_done_int_sts_0_MASK,
	IRQ_DMA0_RX_RING1_WED = WF_WFDMA_EXT_WRAP_CSR_WED_HOST_INT_STA_host_dma0_rx_done_int_sts_1_MASK,
	IRQ_DMA1_RX_RING0_WED = WF_WFDMA_EXT_WRAP_CSR_WED_HOST_INT_STA_host_dma1_rx_done_int_sts_0_MASK,
	IRQ_DMA1_RX_RING1_WED = WF_WFDMA_EXT_WRAP_CSR_WED_HOST_INT_STA_host_dma1_rx_done_int_sts_1_MASK,
	IRQ_LUMP_ALL =
		(IRQ_DMA1_TX_RING16 | IRQ_DMA1_TX_RING17 | IRQ_DMA1_TX_RING18 |
		 IRQ_DMA1_TX_RING19 | IRQ_DMA1_TX_RING20 | IRQ_DMA0_RX_RING0 |
		 IRQ_DMA0_RX_RING1 | IRQ_DMA1_RX_RING0 | IRQ_DMA1_RX_RING1 |
		 IRQ_DMA1_RX_RING2 |
		 WF_WFDMA_EXT_WRAP_CSR_WFDMA_HOST_INT_STA_host_dma0_tx_coherent_int_sts_MASK |
		 WF_WFDMA_EXT_WRAP_CSR_WFDMA_HOST_INT_STA_host_dma1_tx_coherent_int_sts_MASK |
		 WF_WFDMA_EXT_WRAP_CSR_WFDMA_HOST_INT_STA_host_dma0_rx_coherent_int_sts_MASK |
		 WF_WFDMA_EXT_WRAP_CSR_WFDMA_HOST_INT_STA_host_dma1_rx_coherent_int_sts_MASK |
		 WF_WFDMA_EXT_WRAP_CSR_WFDMA_HOST_INT_STA_host_dma1_mcu2host_sw_int_sts_MASK),
	IRQ_LUMP_ALL_WED =
		(IRQ_DMA1_TX_RING0_WED | IRQ_DMA1_TX_RING1_WED | IRQ_DMA1_TX_RING16_WED |
		 IRQ_DMA1_TX_RING17_WED | IRQ_DMA1_TX_RING20_WED | IRQ_DMA0_RX_RING0_WED |
		 IRQ_DMA0_RX_RING1_WED | IRQ_DMA1_RX_RING0_WED | IRQ_DMA1_RX_RING1_WED |
		 WF_WFDMA_EXT_WRAP_CSR_WED_HOST_INT_STA_host_dma0_tx_coherent_int_sts_MASK |
		 WF_WFDMA_EXT_WRAP_CSR_WED_HOST_INT_STA_host_dma1_tx_coherent_int_sts_MASK |
		 WF_WFDMA_EXT_WRAP_CSR_WED_HOST_INT_STA_host_dma0_rx_coherent_int_sts_MASK |
		 WF_WFDMA_EXT_WRAP_CSR_WED_HOST_INT_STA_host_dma1_rx_coherent_int_sts_MASK |
		 WF_WFDMA_EXT_WRAP_CSR_WED_HOST_INT_STA_host_dma1_mcu2host_sw_int_sts_MASK),
};

enum mt7916_irq_mask {
	IRQ_PCIE1_DMA1_TX_RING19 = WF_WFDMA_EXT_WRAP_CSR_PCIE1_HOST_INT_STA_host_dma1_tx_done_int_sts_19_MASK,
	IRQ_PCIE1_DMA0_RX_RING0 = WF_WFDMA_EXT_WRAP_CSR_PCIE1_HOST_INT_STA_host_dma0_rx_don_int_sts_1_MASK,
	IRQ_PCIE1_DMA1_RX_RING2 = WF_WFDMA_EXT_WRAP_CSR_PCIE1_HOST_INT_STA_host_dma1_rx_done_int_sts_2_MASK,
	IRQ_PCIE1_LUMP_ALL =
		(IRQ_PCIE1_DMA1_TX_RING19 | IRQ_PCIE1_DMA0_RX_RING0 | IRQ_PCIE1_DMA1_RX_RING2 |
		 WF_WFDMA_EXT_WRAP_CSR_PCIE1_HOST_INT_STA_host_dma0_tx_coherent_int_sts_MASK |
		 WF_WFDMA_EXT_WRAP_CSR_PCIE1_HOST_INT_STA_host_dma1_rx_coherent_int_sts_MASK |
		 WF_WFDMA_EXT_WRAP_CSR_PCIE1_HOST_INT_STA_host_dma0_rx_coherent_int_sts_MASK |
		 WF_WFDMA_EXT_WRAP_CSR_PCIE1_HOST_INT_STA_host_dma1_rx_coherent_int_sts_MASK),
};

const struct pci_tx_queue_desc txq_layout[] = {
	{
		.cmm = {
			.hw_desc_base = WF_WFDMA_HOST_DMA1_WPDMA_TX_RING16_CTRL0_ADDR,
			.hw_int_mask = WF_WFDMA_EXT_WRAP_CSR_WFDMA_HOST_INT_STA_host_dma1_tx_done_int_sts_16_MASK,
			.q_size = 128,
			.q_attr = Q_TX_FWDL,
			.desc_size = TXD_SIZE,
			.q_info = "FWDL"
		},
	},
	{
		.cmm = {
			.hw_desc_base = WF_WFDMA_HOST_DMA1_WPDMA_TX_RING17_CTRL0_ADDR,
			.hw_int_mask = WF_WFDMA_EXT_WRAP_CSR_WFDMA_HOST_INT_STA_host_dma1_tx_done_int_sts_17_MASK,
			.q_size = 256,
			.q_attr = Q_TX_CMD_WM,
			.desc_size = TXD_SIZE,
			.q_info = "cmd to WM"
		},
	},
	{
		.cmm = {
			.hw_desc_base = WF_WFDMA_HOST_DMA1_WPDMA_TX_RING18_CTRL0_ADDR,
			.hw_int_mask = WF_WFDMA_EXT_WRAP_CSR_WFDMA_HOST_INT_STA_host_dma1_tx_done_int_sts_18_MASK,
			.q_size = 2048,
			.q_attr = Q_TX_DATA,
			.band_idx_bmp = BIT(BAND0),
			.desc_size = TXD_SIZE,
			.q_info = "band0 TXD"
		},
	},
	{
		.cmm = {
			.hw_desc_base = WF_WFDMA_HOST_DMA1_WPDMA_TX_RING19_CTRL0_ADDR,
			.hw_int_mask = WF_WFDMA_EXT_WRAP_CSR_WFDMA_HOST_INT_STA_host_dma1_tx_done_int_sts_19_MASK,
			.q_size = 2048,
			.q_attr = Q_TX_DATA,
			.desc_size = TXD_SIZE,
			.band_idx_bmp = BIT(BAND1),
			.q_info = "band1 TXD"
		},
	},
	{
		.cmm = {
			.hw_desc_base = WF_WFDMA_HOST_DMA1_WPDMA_TX_RING20_CTRL0_ADDR,
			.hw_int_mask = WF_WFDMA_EXT_WRAP_CSR_WFDMA_HOST_INT_STA_host_dma1_tx_done_int_sts_20_MASK,
			.q_size = 256,
			.q_attr = Q_TX_CMD,
			.desc_size = TXD_SIZE,
			.q_info = "cmd to WA"
		},
	}
};

const struct pci_tx_queue_desc txq_wed_layout[] = {
	{
		.cmm = {
			.hw_desc_base = WF_WFDMA_EXT_WRAP_CSR_WED_TX0_CTRL0_ADDR,
			.hw_int_mask = WF_WFDMA_EXT_WRAP_CSR_WED_HOST_INT_STA_host_dma1_tx_done_int_sts_0_MASK,
			.q_size = 2048,
			.q_attr = Q_TX_DATA,
			.band_idx_bmp = BIT(BAND0),
			.desc_size = TXD_SIZE,
			.q_info = "band0 TXD"
		},
	},
	{
		.cmm = {
			.hw_desc_base = WF_WFDMA_EXT_WRAP_CSR_WED_TX1_CTRL0_ADDR,
			.hw_int_mask = WF_WFDMA_EXT_WRAP_CSR_WED_HOST_INT_STA_host_dma1_tx_done_int_sts_1_MASK,
			.q_size = 2048,
			.q_attr = Q_TX_DATA,
			.band_idx_bmp = BIT(BAND1),
			.desc_size = TXD_SIZE,
			.q_info = "band1 TXD"
		},
	},
	{
		.cmm = {
			.hw_desc_base = WF_WFDMA_HOST_DMA1_WPDMA_TX_RING16_CTRL0_ADDR,
			.hw_int_mask = WF_WFDMA_EXT_WRAP_CSR_WED_HOST_INT_STA_host_dma1_tx_done_int_sts_16_MASK,
			.q_size = 128,
			.q_attr = Q_TX_FWDL,
			.desc_size = TXD_SIZE,
			.q_info = "FWDL"
		},
	},
	{
		.cmm = {
			.hw_desc_base = WF_WFDMA_HOST_DMA1_WPDMA_TX_RING17_CTRL0_ADDR,
			.hw_int_mask = WF_WFDMA_EXT_WRAP_CSR_WED_HOST_INT_STA_host_dma1_tx_done_int_sts_17_MASK,
			.q_size = 256,
			.q_attr = Q_TX_CMD_WM,
			.desc_size = TXD_SIZE,
			.q_info = "cmd to WM"
		},
	},
	{
		.cmm = {
			.hw_desc_base = WF_WFDMA_HOST_DMA1_WPDMA_TX_RING20_CTRL0_ADDR,
			.hw_int_mask = WF_WFDMA_EXT_WRAP_CSR_WED_HOST_INT_STA_host_dma1_tx_done_int_sts_20_MASK,
			.q_size = 256,
			.q_attr = Q_TX_CMD,
			.desc_size = TXD_SIZE,
			.q_info = "cmd to WA"
		},
	}
};

const struct pci_rx_queue_desc rxq_layout[] = {
	{
		.cmm = {
			.hw_desc_base = WF_WFDMA_HOST_DMA0_WPDMA_RX_RING0_CTRL0_ADDR,
			.hw_int_mask = WF_WFDMA_EXT_WRAP_CSR_WFDMA_HOST_INT_STA_host_dma0_rx_don_int_sts_0_MASK,
			.hw_didx_mask = WF_WFDMA_HOST_DMA0_WPDMA_RX_RING0_CTRL3_DMA_IDX_MASK,
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
			.hw_desc_base = WF_WFDMA_HOST_DMA0_WPDMA_RX_RING1_CTRL0_ADDR,
			.hw_int_mask = WF_WFDMA_EXT_WRAP_CSR_WFDMA_HOST_INT_STA_host_dma0_rx_don_int_sts_1_MASK,
			.hw_didx_mask = WF_WFDMA_HOST_DMA0_WPDMA_RX_RING1_CTRL3_DMA_IDX_MASK,
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
			.hw_desc_base = WF_WFDMA_HOST_DMA1_WPDMA_RX_RING0_CTRL0_ADDR,
			.hw_int_mask = WF_WFDMA_EXT_WRAP_CSR_WFDMA_HOST_INT_STA_host_dma1_rx_done_int_sts_0_MASK,
			.hw_didx_mask = WF_WFDMA_HOST_DMA1_WPDMA_RX_RING0_CTRL3_DMA_IDX_MASK,
			.q_size = 512,
			.q_attr = Q_RX_EVENT_WM,
			.desc_size = RXD_SIZE,
			.q_info = "event from WM",
		},
		.rx_buf_size = RX_BUF_SIZE,
	},
	{
		.cmm = {
			.hw_desc_base = WF_WFDMA_HOST_DMA1_WPDMA_RX_RING1_CTRL0_ADDR,
			.hw_int_mask = WF_WFDMA_EXT_WRAP_CSR_WFDMA_HOST_INT_STA_host_dma1_rx_done_int_sts_1_MASK,
			.hw_didx_mask = WF_WFDMA_HOST_DMA1_WPDMA_RX_RING1_CTRL3_DMA_IDX_MASK,
			.q_size = 1024,
			.q_attr = Q_RX_EVENT_TX_FREE_DONE,
			.desc_size = RXD_SIZE,
			.band_idx_bmp = BIT(BAND0),
			.q_info = "event from WA band0",
		},
		.rx_buf_size = RX_BUF_SIZE,
	},
	{
		.cmm = {
			.hw_desc_base = WF_WFDMA_HOST_DMA1_WPDMA_RX_RING2_CTRL0_ADDR,
			.hw_int_mask = WF_WFDMA_EXT_WRAP_CSR_WFDMA_HOST_INT_STA_host_dma1_rx_done_int_sts_2_MASK,
			.hw_didx_mask = WF_WFDMA_HOST_DMA1_WPDMA_RX_RING2_CTRL3_DMA_IDX_MASK,
			.q_size = 512,
			.q_attr = Q_RX_EVENT_TX_FREE_DONE,
			.desc_size = RXD_SIZE,
			.band_idx_bmp = BIT(BAND1),
			.q_info = "event from WA band1",
		},
		.rx_buf_size = RX_BUF_SIZE,
	}
};

const struct pci_rx_queue_desc rxq_wed_layout[] = {
	{
		.cmm = {
			.hw_desc_base = WF_WFDMA_HOST_DMA0_WPDMA_RX_RING0_CTRL0_ADDR,
			.hw_int_mask = WF_WFDMA_EXT_WRAP_CSR_WED_HOST_INT_STA_host_dma0_rx_done_int_sts_0_MASK,
			.hw_didx_mask = WF_WFDMA_HOST_DMA0_WPDMA_RX_RING0_CTRL3_DMA_IDX_MASK,
			.q_size = 1536,
			.q_attr = Q_RX_DATA,
			.band_idx_bmp = BIT(BAND0),
			.desc_size = RXD_SIZE,
			.q_info = "band0 RX data",
		},
		.rx_buf_size = RX_BUF_SIZE,
	},
	{
		.cmm = {
			.hw_desc_base = WF_WFDMA_HOST_DMA0_WPDMA_RX_RING1_CTRL0_ADDR,
			.hw_int_mask = WF_WFDMA_EXT_WRAP_CSR_WED_HOST_INT_STA_host_dma0_rx_done_int_sts_1_MASK,
			.hw_didx_mask = WF_WFDMA_HOST_DMA0_WPDMA_RX_RING1_CTRL3_DMA_IDX_MASK,
			.q_size = 1536,
			.q_attr = Q_RX_DATA,
			.band_idx_bmp = BIT(BAND1),
			.desc_size = RXD_SIZE,
			.q_info = "band1 RX data",
		},
		.rx_buf_size = RX_BUF_SIZE,
	},
	{
		.cmm = {
			.hw_desc_base = WF_WFDMA_HOST_DMA1_WPDMA_RX_RING0_CTRL0_ADDR,
			.hw_int_mask = WF_WFDMA_EXT_WRAP_CSR_WED_HOST_INT_STA_host_dma1_rx_done_int_sts_0_MASK,
			.hw_didx_mask = WF_WFDMA_HOST_DMA1_WPDMA_RX_RING0_CTRL3_DMA_IDX_MASK,
			.q_size = 512,
			.q_attr = Q_RX_EVENT_WM,
			.desc_size = RXD_SIZE,
			.q_info = "event from WM",
		},
		.rx_buf_size = RX_BUF_SIZE,
	},
	{
		.cmm = {
			.hw_desc_base = WF_WFDMA_EXT_WRAP_CSR_WED_RX1_CTRL0_ADDR,
			.hw_int_mask = WF_WFDMA_EXT_WRAP_CSR_WED_HOST_INT_STA_host_dma1_rx_done_int_sts_1_MASK,
			.hw_didx_mask = WF_WFDMA_EXT_WRAP_CSR_WED_RX1_STS_WED_RX_RING1_DIDX_MASK,
			.q_size = 1024,
			.q_attr = Q_RX_EVENT_TX_FREE_DONE,
			.desc_size = RXD_SIZE,
			.band_idx_bmp = BIT(BAND0),
			.q_info = "event from WA band0",
		},
		.rx_buf_size = RX_BUF_SIZE,
		.free_done_handled_by = MASTER_HANDLER,
	},
};

struct pci_tx_queue_desc txq_layout_pcie1[] = {
	{
		.cmm = {
			.hw_desc_base = WF_WFDMA_HOST_DMA1_PCIE1_WPDMA_TX_RING19_CTRL0_ADDR,
			.hw_int_mask = WF_WFDMA_EXT_WRAP_CSR_PCIE1_HOST_INT_STA_host_dma1_tx_done_int_sts_19_MASK,
			.q_size = 2048,
			.q_attr = Q_TX_DATA,
			.desc_size = TXD_SIZE,
			.q_info = "band1 TXD",
		},
	 },
};


const struct pci_rx_queue_desc rxq_layout_pcie1[] = {
	{
		.cmm = {
			.hw_desc_base = WF_WFDMA_HOST_DMA0_PCIE1_WPDMA_RX_RING1_CTRL0_ADDR,
			.hw_int_mask = WF_WFDMA_EXT_WRAP_CSR_PCIE1_HOST_INT_STA_host_dma0_rx_don_int_sts_1_MASK,
			.hw_didx_mask = WF_WFDMA_HOST_DMA0_PCIE1_WPDMA_RX_RING1_CTRL3_DMA_IDX_MASK,
			.q_size = 1536,
			.q_attr = Q_RX_DATA,
			.desc_size = RXD_SIZE,
			.band_idx_bmp = BIT(BAND1),
			.q_info = "band1 data",
		},
		.rx_buf_size = RX_BUF_SIZE,
	},
	{
		.cmm = {
			.hw_desc_base = WF_WFDMA_HOST_DMA1_PCIE1_WPDMA_RX_RING2_CTRL0_ADDR,
			.hw_int_mask = WF_WFDMA_EXT_WRAP_CSR_PCIE1_HOST_INT_STA_host_dma1_rx_done_int_sts_2_MASK,
			.hw_didx_mask = WF_WFDMA_HOST_DMA1_PCIE1_WPDMA_RX_RING2_CTRL3_DMA_IDX_MASK,
			.q_size = 1024,
			.q_attr = Q_RX_EVENT_TX_FREE_DONE,
			.desc_size = RXD_SIZE,
			.band_idx_bmp = BIT(BAND1),
			.q_info = "event from WA band1",
		},
		.rx_buf_size = RX_BUF_SIZE,
		.free_done_handled_by = SLAVE_HANDLER,
	}
};

struct pci_chip_profile mt7915_pci_profile = {
	.int_mask_map = {
		/* map_idx 0: Legacy interrupt bitmap */
		[0] = {
			.int_sts_addr = WF_WFDMA_EXT_WRAP_CSR_WFDMA_HOST_INT_STA_ADDR,
			.int_ena_addr = WF_WFDMA_EXT_WRAP_CSR_WFDMA_HOST_INT_ENA_ADDR,
			.int_ena_mask = IRQ_LUMP_ALL,
		},
	},
	.lp_fw_own_reg_addr = CONN_HOST_CSR_TOP_WF_BAND0_LPCTL_conn_wf_b0_host_lpcr_fw_own_sync_ADDR,
	.txq_num = ARRAY_SIZE(txq_layout),
	.rxq_num = ARRAY_SIZE(rxq_layout),
	.queue_layout = {
		.tx_queue_layout = txq_layout,
		.rx_queue_layout = rxq_layout,
	},
	/*mcu2host swi*/
	.int_enable_swi_mask =
		WF_WFDMA_EXT_WRAP_CSR_WFDMA_HOST_INT_STA_host_dma1_mcu2host_sw_int_sts_MASK,
	.mcu2host_sw_int_ena_addr = WF_WFDMA_HOST_DMA0_MCU2HOST_SW_INT_ENA_ADDR,
	.mcu2host_sw_int_sta_addr = WF_WFDMA_HOST_DMA0_MCU2HOST_SW_INT_STA_ADDR,
	.mcu2host_sw_int_ser_mask = (
	WF_WFDMA_HOST_DMA0_MCU2HOST_SW_INT_STA_mcu2host_sw_int_2_MASK |
	WF_WFDMA_HOST_DMA0_MCU2HOST_SW_INT_STA_mcu2host_sw_int_3_MASK |
	WF_WFDMA_HOST_DMA0_MCU2HOST_SW_INT_STA_mcu2host_sw_int_4_MASK |
	WF_WFDMA_HOST_DMA0_MCU2HOST_SW_INT_STA_mcu2host_sw_int_5_MASK),
};

struct pci_chip_profile mt7915_pci_wed_profile = {
	.int_mask_map = {
		/* map_idx 0: Legacy interrupt bitmap */
		[0] = {
			.int_sts_addr = WF_WFDMA_EXT_WRAP_CSR_WED_HOST_INT_STA_ADDR,
			.int_ena_addr = WF_WFDMA_EXT_WRAP_CSR_WED_HOST_INT_ENA_ADDR,
			.int_ena_mask = IRQ_LUMP_ALL_WED,
		},
	},
	.lp_fw_own_reg_addr = CONN_HOST_CSR_TOP_WF_BAND0_LPCTL_conn_wf_b0_host_lpcr_fw_own_sync_ADDR,
	.dma_addr = WF_WFDMA_EXT_WRAP_CSR_BASE,
	.tx_dma_glo_reg_addr = WF_WFDMA_HOST_DMA1_WPDMA_GLO_CFG_ADDR,
	.rx_dma_glo_reg_addr = WF_WFDMA_HOST_DMA0_WPDMA_GLO_CFG_ADDR,
	.int_ser_reg_addr = WF_WFDMA_MCU_DMA1_HOST2MCU_SW_INT_SET_ADDR,
	.int_ser_mask = BIT(2),
	.txq_num = ARRAY_SIZE(txq_wed_layout),
	.rxq_num = ARRAY_SIZE(rxq_wed_layout),
	.queue_layout = {
		.tx_queue_layout = txq_wed_layout,
		.rx_queue_layout = rxq_wed_layout,
	},
};

struct pci_chip_profile mt7916_pci_profile = {
	.int_mask_map = {
		/* map_idx 0: Legacy interrupt bitmap */
		[0] = {
			.int_sts_addr = WF_WFDMA_EXT_WRAP_CSR_PCIE1_HOST_INT_STA_ADDR,
			.int_ena_addr = WF_WFDMA_EXT_WRAP_CSR_PCIE1_HOST_INT_ENA_ADDR,
			.int_ena_mask = IRQ_PCIE1_LUMP_ALL,
		},
	},
	.lp_fw_own_reg_addr = CONN_HOST_CSR_TOP_WF_BAND0_LPCTL_conn_wf_b0_host_lpcr_fw_own_sync_ADDR,
	.txq_num = ARRAY_SIZE(txq_layout_pcie1),
	.rxq_num = ARRAY_SIZE(rxq_layout_pcie1),
	.queue_layout = {
		.tx_queue_layout = txq_layout_pcie1,
		.rx_queue_layout = rxq_layout_pcie1,
	},
};

#endif
