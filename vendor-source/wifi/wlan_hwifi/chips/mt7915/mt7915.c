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

#include <linux/kernel.h>
#include <linux/module.h>
#include "mt7915.h"
#include "mac_if.h"
#include "core.h"
#include "firmware/mt7915_e2p_ePAeLNA.h"


static int mac_type = MTK_INTERFACE_CONNAC;
module_param(mac_type, int, 0644);
MODULE_PARM_DESC(mac_type, "mac driver type");

#define MT7915_CHIP_ID 0x7915

static void
dump_chip_info(struct pci_trans *pci_hw)
{
#define MT_HW_CHIPID			0x70010200
#define MT_HW_REV				0x70010204
#define MT_HW_CODE				0x70010200
	struct mtk_bus_trans *trans = to_bus_trans(pci_hw);
	u32 chipid = mt7915_l1_rr(trans, MT_HW_CHIPID);
	u32 rev = mt7915_l1_rr(trans, MT_HW_REV);
	u32 code = mt7915_l1_rr(trans, MT_HW_CODE);
	u32 ena = mt7915_l1_rr(trans, CONN_HOST_CSR_TOP_WF_BAND0_IRQ_ENA_conn_hif_on_wf_b0_irq_ena_ADDR);
	u32 lp = mt7915_l1_rr(trans, CONN_HOST_CSR_TOP_WF_BAND1_LPCTL_conn_wf_b1_host_lpcr_fw_own_sync_ADDR);

	dev_info(to_device(trans),
		"%s(): chip id: %x, rev: %x, code: %x, ena=%x, lp:%x\n",
		__func__, chipid, rev, code, ena, lp);
}

static void
_mt7915_pdma_irq_init(struct pci_trans *pci_hw)
{
	struct mtk_bus_trans *trans = to_bus_trans(pci_hw);
	u32 val;
	/*
	 * Ownership interrupt setting
	 */
	/* CONN_HOST_CSR_TOP */
	val = mt7915_l1_rr(trans, CONN_HOST_CSR_TOP_WF_BAND0_IRQ_ENA_conn_hif_on_wf_b0_irq_ena_ADDR);
	val |= (1 << CONN_HOST_CSR_TOP_WF_BAND0_IRQ_ENA_conn_hif_on_wf_b0_irq_ena_SHFT);
	mt7915_l1_wr(trans, CONN_HOST_CSR_TOP_WF_BAND0_IRQ_ENA_conn_hif_on_wf_b0_irq_ena_ADDR, val);

	val = mt7915_l1_rr(trans, CONN_HOST_CSR_TOP_WF_BAND1_IRQ_ENA_conn_hif_on_wf_b1_irq_ena_ADDR);
	val |= (1 << CONN_HOST_CSR_TOP_WF_BAND1_IRQ_ENA_conn_hif_on_wf_b1_irq_ena_SHFT);
	mt7915_l1_wr(trans, CONN_HOST_CSR_TOP_WF_BAND1_IRQ_ENA_conn_hif_on_wf_b1_irq_ena_ADDR, val);

	/*
	 * SUBSYS2HOST interrupt setting
	 */
	/* 1. WFDMA_HOST_INT_ENA bit28, handled in mt7915_hif_ctrl_chip_init */
	/* 2. HOST_INT_ENA bit28 */
	bus_set(trans, WF_WFDMA_HOST_DMA1_HOST_INT_ENA_subsys_int_ena_ADDR,
		WF_WFDMA_HOST_DMA1_HOST_INT_ENA_subsys_int_ena_MASK);
	/* 3. SUBSYS2HOST_INT_ENA bit8  */
	bus_set(trans, WF_WFDMA_HOST_DMA1_SUBSYS2HOST_INT_ENA_conn_hif_on_host_int_ena_ADDR,
		WF_WFDMA_HOST_DMA1_SUBSYS2HOST_INT_ENA_conn_hif_on_host_int_ena_MASK);

	/*
	 * MCU2HOST SWI setting
	 */
	/* 1. WFDMA_HOST_INT_ENA bit29, handled in mt7915_hif_ctrl_chip_init */
	/* 2. HOST_INT_ENA bit29 */
	bus_set(trans, WF_WFDMA_HOST_DMA1_HOST_INT_ENA_mcu2host_sw_int_ena_ADDR,
		WF_WFDMA_HOST_DMA1_HOST_INT_ENA_mcu2host_sw_int_ena_MASK);
	/* 3. MCU2HOST_SW_INT_ENA */
	bus_set(trans, WF_WFDMA_HOST_DMA1_MCU2HOST_SW_INT_ENA_ADDR,
		WF_WFDMA_HOST_DMA1_MCU2HOST_SW_INT_ENA_mcu2host_int_ena_0_MASK |
		WF_WFDMA_HOST_DMA1_MCU2HOST_SW_INT_ENA_mcu2host_int_ena_1_MASK);

	/* priority interrupt setting */
	bus_set(trans, WF_WFDMA_HOST_DMA0_WPDMA_INT_RX_PRI_SEL_ADDR,
		WF_WFDMA_HOST_DMA0_WPDMA_INT_RX_PRI_SEL_WPDMA_INT_RX_RING0_PRI_SEL_MASK |
		WF_WFDMA_HOST_DMA0_WPDMA_INT_RX_PRI_SEL_WPDMA_INT_RX_RING1_PRI_SEL_MASK);

	bus_set(trans, WF_WFDMA_HOST_DMA1_WPDMA_INT_RX_PRI_SEL_ADDR,
		WF_WFDMA_HOST_DMA1_WPDMA_INT_RX_PRI_SEL_WPDMA_INT_RX_RING0_PRI_SEL_MASK |
		WF_WFDMA_HOST_DMA1_WPDMA_INT_RX_PRI_SEL_WPDMA_INT_RX_RING1_PRI_SEL_MASK |
		WF_WFDMA_HOST_DMA1_WPDMA_INT_RX_PRI_SEL_WPDMA_INT_RX_RING2_PRI_SEL_MASK);

	bus_set(trans, WF_WFDMA_HOST_DMA1_WPDMA_INT_TX_PRI_SEL_ADDR,
		WF_WFDMA_HOST_DMA1_WPDMA_INT_TX_PRI_SEL_WPDMA_INT_TX_RING18_PRI_SEL_MASK |
		WF_WFDMA_HOST_DMA1_WPDMA_INT_TX_PRI_SEL_WPDMA_INT_TX_RING19_PRI_SEL_MASK);
}

static void
_mt7915_irq_disable(struct pci_trans *pci_hw)
{
	struct mtk_bus_trans *trans = to_bus_trans(pci_hw);
	unsigned long flags;
	u32 mac_int_addr = MT_PCIE_MAC_INT_ENABLE_ADDR;

	spin_lock_irqsave(&pci_hw->irq_lock, flags);
	bus_write(trans, pci_hw->int_mask_map[0].int_ena_addr, 0);
	mt7915_l1_wr(trans, mac_int_addr, 0x0);
	spin_unlock_irqrestore(&pci_hw->irq_lock, flags);
	clear_bit(PCI_STATE_ISR_START, &pci_hw->state);
}

static int
_mt7915_irq_enable(struct pci_trans *pci_hw)
{
	struct mtk_bus_trans *trans = to_bus_trans(pci_hw);
	unsigned long flags;
	u32 mac_int_addr = MT_PCIE_MAC_INT_ENABLE_ADDR;

	spin_lock_irqsave(&pci_hw->irq_lock, flags);

	bus_write(trans, pci_hw->int_mask_map[0].int_sts_addr, 0xffffffff);
	bus_write(trans, pci_hw->int_mask_map[0].int_ena_addr, 0);
	/* clear garbage ints */
	bus_write(trans, pci_hw->int_mask_map[0].int_ena_addr,
			 pci_hw->int_mask_map[0].int_ena_mask);
	/* Master Switch of PCIE Interrupt Enable */
	mt7915_l1_wr(trans, mac_int_addr, 0x000000ff);
	spin_unlock_irqrestore(&pci_hw->irq_lock, flags);
	clear_bit(PCI_STATE_ISR_PEDING, &pci_hw->state);
	set_bit(PCI_STATE_ISR_START, &pci_hw->state);
	return 0;
}

static void
_mt7915_pdma_init(struct pci_trans *pci_hw)
{
	struct mtk_bus_trans *trans = to_bus_trans(pci_hw);
	u32 val;
	/* configure global setting */
	/* host DMA1(to WMCPU/WACPU) need to omit tx/rx info */
	bus_set(trans, WF_WFDMA_HOST_DMA1_WPDMA_GLO_CFG_ADDR,
			WF_WFDMA_HOST_DMA1_WPDMA_GLO_CFG_OMIT_TX_INFO_MASK |
			WF_WFDMA_HOST_DMA1_WPDMA_GLO_CFG_OMIT_RX_INFO_MASK |
			WF_WFDMA_HOST_DMA1_WPDMA_GLO_CFG_PDMA_ADDR_EXT_EN_MASK);

	/* configure prefetch setting */
	if (test_bit(PCI_FLAG_PREFETCH_MANUAL, &pci_hw->flags)) {
		bus_clear(trans, WF_WFDMA_HOST_DMA0_WPDMA_GLO_CFG_ADDR,
			WF_WFDMA_HOST_DMA0_WPDMA_GLO_CFG_CSR_DISP_BASE_PTR_CHAIN_EN_MASK);
		/* disable prefetch offset calculation auto-mode */
		bus_clear(trans, WF_WFDMA_HOST_DMA1_WPDMA_GLO_CFG_ADDR,
			WF_WFDMA_HOST_DMA1_WPDMA_GLO_CFG_CSR_DISP_BASE_PTR_CHAIN_EN_MASK);
	}

	/*keep rxdmad info*/
	bus_set(trans,
		WF_WFDMA_HOST_DMA0_WPDMA_GLO_CFG_EXT0_ADDR,
		WF_WFDMA_HOST_DMA0_WPDMA_GLO_CFG_EXT0_CSR_RX_WB_KEEP_RSVD_MASK);

	bus_set(trans,
		WF_WFDMA_HOST_DMA1_WPDMA_GLO_CFG_EXT0_ADDR,
		WF_WFDMA_HOST_DMA1_WPDMA_GLO_CFG_EXT0_CSR_RX_WB_KEEP_RSVD_MASK);

	bus_write(trans, WF_WFDMA_HOST_DMA0_WPDMA_RX_RING0_EXT_CTRL_ADDR, 0x00000004);
	bus_write(trans, WF_WFDMA_HOST_DMA0_WPDMA_RX_RING1_EXT_CTRL_ADDR, 0x00400004);
	bus_write(trans, WF_WFDMA_HOST_DMA0_WPDMA_RX_RING2_EXT_CTRL_ADDR, 0x00800000);

	bus_write(trans, WF_WFDMA_HOST_DMA1_WPDMA_TX_RING0_EXT_CTRL_ADDR, 0x00800004);
	bus_write(trans, WF_WFDMA_HOST_DMA1_WPDMA_TX_RING1_EXT_CTRL_ADDR, 0x00c00004);
	bus_write(trans, WF_WFDMA_HOST_DMA1_WPDMA_TX_RING2_EXT_CTRL_ADDR, 0x01000004);
	bus_write(trans, WF_WFDMA_HOST_DMA1_WPDMA_TX_RING3_EXT_CTRL_ADDR, 0x01400004);
	bus_write(trans, WF_WFDMA_HOST_DMA1_WPDMA_TX_RING4_EXT_CTRL_ADDR, 0x01800004);
	bus_write(trans, WF_WFDMA_HOST_DMA1_WPDMA_TX_RING5_EXT_CTRL_ADDR, 0x01c00004);
	bus_write(trans, WF_WFDMA_HOST_DMA1_WPDMA_TX_RING6_EXT_CTRL_ADDR, 0x02000004);
	bus_write(trans, WF_WFDMA_HOST_DMA1_WPDMA_TX_RING7_EXT_CTRL_ADDR, 0x02400004);

	bus_write(trans, WF_WFDMA_HOST_DMA1_WPDMA_TX_RING16_EXT_CTRL_ADDR, 0x02800004);
	bus_write(trans, WF_WFDMA_HOST_DMA1_WPDMA_TX_RING17_EXT_CTRL_ADDR, 0x02c00004);
	bus_write(trans, WF_WFDMA_HOST_DMA1_WPDMA_TX_RING18_EXT_CTRL_ADDR, 0x03000004);
	bus_write(trans, WF_WFDMA_HOST_DMA1_WPDMA_TX_RING19_EXT_CTRL_ADDR, 0x03400004);
	bus_write(trans, WF_WFDMA_HOST_DMA1_WPDMA_TX_RING20_EXT_CTRL_ADDR, 0x03800004);
	bus_write(trans, WF_WFDMA_HOST_DMA1_WPDMA_TX_RING21_EXT_CTRL_ADDR, 0x03c00000);

	bus_write(trans, WF_WFDMA_HOST_DMA1_WPDMA_RX_RING0_EXT_CTRL_ADDR, 0x03c00004);
	bus_write(trans, WF_WFDMA_HOST_DMA1_WPDMA_RX_RING1_EXT_CTRL_ADDR, 0x04000004);
	bus_write(trans, WF_WFDMA_HOST_DMA1_WPDMA_RX_RING2_EXT_CTRL_ADDR, 0x04400004);
	bus_write(trans, WF_WFDMA_HOST_DMA1_WPDMA_RX_RING3_EXT_CTRL_ADDR, 0x04800000);

	/* reset dma idx */
	bus_write(trans, WF_WFDMA_HOST_DMA0_WPDMA_RST_DTX_PTR_ADDR, 0xffffffff);
	bus_write(trans, WF_WFDMA_HOST_DMA1_WPDMA_RST_DTX_PTR_ADDR, 0xffffffff);

	/* configure delay interrupt */
	if (test_bit(PCI_FLAG_DELAY_INTR, &pci_hw->flags)) {
		/* enable hif_dma0 pri 1(rx data) */
		bus_write(trans, WF_WFDMA_HOST_DMA0_WPDMA_PRI_DLY_INT_CFG0_ADDR,
			(1	<< WF_WFDMA_HOST_DMA0_WPDMA_PRI_DLY_INT_CFG0_PRI1_DLY_INT_EN_SHFT) |
			(1	<< WF_WFDMA_HOST_DMA0_WPDMA_PRI_DLY_INT_CFG0_PRI1_MAX_PINT_SHFT) |
			(0x1c << WF_WFDMA_HOST_DMA0_WPDMA_PRI_DLY_INT_CFG0_PRI1_MAX_PTIME_SHFT));
		bus_write(trans, WF_WFDMA_HOST_DMA1_WPDMA_PRI_DLY_INT_CFG0_ADDR, 0);
	}

	if (test_bit(PCI_FLAG_WHNAT_TX, &pci_hw->flags) &&
		(!test_bit(PCI_FLAG_WHNAT_RX, &pci_hw->flags))) {
		/* configure WED ring alias */
		bus_set(trans, WF_WFDMA_EXT_WRAP_CSR_WFDMA_HOST_CONFIG_ADDR,
				WF_WFDMA_EXT_WRAP_CSR_WFDMA_HOST_CONFIG_host_wed_en_MASK);

		val = bus_read(trans, WF_WFDMA_EXT_WRAP_CSR_WED_RING_CONTROL_IDX_ADDR);
		val &= ~WF_WFDMA_EXT_WRAP_CSR_WED_RING_CONTROL_IDX_WED_TX_RING0_CONTROL_IDX_MASK;
		val |= 18 << WF_WFDMA_EXT_WRAP_CSR_WED_RING_CONTROL_IDX_WED_TX_RING0_CONTROL_IDX_SHFT;
		bus_write(trans, WF_WFDMA_EXT_WRAP_CSR_WED_RING_CONTROL_IDX_ADDR, val);

		val = bus_read(trans, WF_WFDMA_EXT_WRAP_CSR_WED_RING_CONTROL_IDX_ADDR);
		val &= ~WF_WFDMA_EXT_WRAP_CSR_WED_RING_CONTROL_IDX_WED_TX_RING1_CONTROL_IDX_MASK;
		val |= 19 << WF_WFDMA_EXT_WRAP_CSR_WED_RING_CONTROL_IDX_WED_TX_RING1_CONTROL_IDX_SHFT;
		bus_write(trans, WF_WFDMA_EXT_WRAP_CSR_WED_RING_CONTROL_IDX_ADDR, val);

		val = bus_read(trans, WF_WFDMA_EXT_WRAP_CSR_WED_RING_CONTROL_IDX_ADDR);
		val &= ~WF_WFDMA_EXT_WRAP_CSR_WED_RING_CONTROL_IDX_WED_RX_RING1_CONTROL_IDX_MASK;
		val |= 0x1 << WF_WFDMA_EXT_WRAP_CSR_WED_RING_CONTROL_IDX_WED_RX_RING1_CONTROL_IDX_SHFT;
		bus_write(trans, WF_WFDMA_EXT_WRAP_CSR_WED_RING_CONTROL_IDX_ADDR, val);
	}

	if (test_bit(PCI_FLAG_CTXD, &pci_hw->flags)) {
		bus_set(trans, WF_WFDMA_HOST_DMA1_WPDMA_GLO_CFG_EXT1_ADDR,
			WF_WFDMA_HOST_DMA1_WPDMA_GLO_CFG_EXT1_CSR_TX_FCTRL_MODE_MASK);
	}
}

static bool
_mt7915_pdma_poll(struct pci_trans *pci_hw, u32 round, u32 wait_us)
{
	struct mtk_bus_trans *trans = to_bus_trans(pci_hw);
	u8 i = 0;
	u32 val = 0;
	bool idle;

	/*band 0*/
	bus_set(trans, WF_WFDMA_HOST_DMA0_CONN_HIF_BUSY_ENA_ADDR,
		WF_WFDMA_HOST_DMA0_CONN_HIF_BUSY_ENA_conn_hif_rxfifo_busy_enable_MASK |
		WF_WFDMA_HOST_DMA0_CONN_HIF_BUSY_ENA_conn_hif_txfifo0_busy_enable_MASK |
		WF_WFDMA_HOST_DMA0_CONN_HIF_BUSY_ENA_conn_hif_txfifo1_busy_enable_MASK);

	bus_set(trans, WF_WFDMA_HOST_DMA1_CONN_HIF_BUSY_ENA_ADDR,
		WF_WFDMA_HOST_DMA1_CONN_HIF_BUSY_ENA_conn_hif_rxfifo_busy_enable_MASK |
		WF_WFDMA_HOST_DMA1_CONN_HIF_BUSY_ENA_conn_hif_txfifo0_busy_enable_MASK |
		WF_WFDMA_HOST_DMA1_CONN_HIF_BUSY_ENA_conn_hif_txfifo1_busy_enable_MASK);

	bus_clear(trans, WF_WFDMA_HOST_DMA0_PCIE1_CONN_HIF_BUSY_ENA_ADDR,
		WF_WFDMA_HOST_DMA0_PCIE1_CONN_HIF_BUSY_ENA_conn_hif_rxfifo_busy_enable_MASK |
		WF_WFDMA_HOST_DMA0_PCIE1_CONN_HIF_BUSY_ENA_conn_hif_txfifo0_busy_enable_MASK |
		WF_WFDMA_HOST_DMA0_PCIE1_CONN_HIF_BUSY_ENA_conn_hif_txfifo1_busy_enable_MASK);

	bus_clear(trans, WF_WFDMA_HOST_DMA1_PCIE1_CONN_HIF_BUSY_ENA_ADDR,
		WF_WFDMA_HOST_DMA1_PCIE1_CONN_HIF_BUSY_ENA_conn_hif_rxfifo_busy_enable_MASK |
		WF_WFDMA_HOST_DMA1_PCIE1_CONN_HIF_BUSY_ENA_conn_hif_txfifo0_busy_enable_MASK |
		WF_WFDMA_HOST_DMA1_PCIE1_CONN_HIF_BUSY_ENA_conn_hif_txfifo1_busy_enable_MASK);

	i = 0;
	idle = false;
	do {
		val = bus_read(trans, WF_WFDMA_EXT_WRAP_CSR_WFDMA_HIF_MISC_ADDR);
		if ((val & WF_WFDMA_EXT_WRAP_CSR_WFDMA_HIF_MISC_HIF_BUSY_MASK) == 0) {
			idle = true;
			break;
		}

		udelay(wait_us);
	} while ((i++) < round);

	return !idle;
}

static void
_mt7915_pdma_reset(struct pci_trans *pci_hw)
{
	struct mtk_bus_trans *trans = to_bus_trans(pci_hw);

	bus_clear(trans, WF_WFDMA_HOST_DMA1_CONN_HIF_RST_ADDR,
		WF_WFDMA_HOST_DMA1_CONN_HIF_RST_conn_hif_logic_rst_n_MASK |
		WF_WFDMA_HOST_DMA1_CONN_HIF_RST_dmashdl_all_rst_n_MASK);

	bus_set(trans, WF_WFDMA_HOST_DMA1_CONN_HIF_RST_ADDR,
		WF_WFDMA_HOST_DMA1_CONN_HIF_RST_conn_hif_logic_rst_n_MASK |
		WF_WFDMA_HOST_DMA1_CONN_HIF_RST_dmashdl_all_rst_n_MASK);

	bus_clear(trans, WF_WFDMA_HOST_DMA0_CONN_HIF_RST_ADDR,
		WF_WFDMA_HOST_DMA0_CONN_HIF_RST_conn_hif_logic_rst_n_MASK |
		WF_WFDMA_HOST_DMA0_CONN_HIF_RST_dmashdl_all_rst_n_MASK);

	bus_set(trans, WF_WFDMA_HOST_DMA0_CONN_HIF_RST_ADDR,
		WF_WFDMA_HOST_DMA0_CONN_HIF_RST_conn_hif_logic_rst_n_MASK |
		WF_WFDMA_HOST_DMA0_CONN_HIF_RST_dmashdl_all_rst_n_MASK);
}

static int
_mt7915_pdma_enable(struct pci_trans *pci_hw)
{
	struct mtk_bus_trans *trans = to_bus_trans(pci_hw);

	bus_set(trans, WF_WFDMA_HOST_DMA0_WPDMA_GLO_CFG_ADDR,
			WF_WFDMA_HOST_DMA0_WPDMA_GLO_CFG_TX_DMA_EN_MASK |
			WF_WFDMA_HOST_DMA0_WPDMA_GLO_CFG_RX_DMA_EN_MASK);

	bus_set(trans, WF_WFDMA_HOST_DMA1_WPDMA_GLO_CFG_ADDR,
			WF_WFDMA_HOST_DMA1_WPDMA_GLO_CFG_TX_DMA_EN_MASK |
			WF_WFDMA_HOST_DMA1_WPDMA_GLO_CFG_RX_DMA_EN_MASK);

	if (test_bit(PCI_FLAG_WHNAT_RX, &pci_hw->flags)) {
		bus_clear(trans, WF_WFDMA_HOST_DMA0_WPDMA_GLO_CFG_ADDR,
		WF_WFDMA_HOST_DMA0_WPDMA_GLO_CFG_RX_DMA_EN_MASK);
	}
	return 0;
}

static int
_mt7915_pdma_disable(struct pci_trans *pci_hw)
{
	struct mtk_bus_trans *trans = to_bus_trans(pci_hw);

	_mt7915_irq_disable(pci_hw);
	bus_clear(trans, WF_WFDMA_HOST_DMA0_WPDMA_GLO_CFG_ADDR,
			WF_WFDMA_HOST_DMA0_WPDMA_GLO_CFG_TX_DMA_EN_MASK |
			WF_WFDMA_HOST_DMA0_WPDMA_GLO_CFG_RX_DMA_EN_MASK);
	bus_clear(trans, WF_WFDMA_HOST_DMA1_WPDMA_GLO_CFG_ADDR,
			WF_WFDMA_HOST_DMA1_WPDMA_GLO_CFG_TX_DMA_EN_MASK |
			WF_WFDMA_HOST_DMA1_WPDMA_GLO_CFG_RX_DMA_EN_MASK);
	return 0;
}

static int
mt7915_pdma_enable(struct pci_trans *trans)
{
	int ret;

#define PDMA_POLL_ROUND 100
#define PDMA_POLL_US 1000

	ret = _mt7915_pdma_poll(trans, PDMA_POLL_ROUND, PDMA_POLL_US);
	if (ret)
		goto err;

	ret = _mt7915_pdma_enable(trans);
err:
	return ret;
}

static int
mt7915_pdma_disable(struct pci_trans *trans)
{
	_mt7915_pdma_disable(trans);
	_mt7915_pdma_reset(trans);
	return 0;
}

static int
mt7915_pdma_init(struct pci_trans *trans)
{
	dump_chip_info(trans);
	_mt7915_pdma_init(trans);
	_mt7915_pdma_irq_init(trans);
	_mt7915_irq_enable(trans);
	return 0;
}

/**
 * @brief chip specific recognation machanism
 *
 * @param *master master trans used to set recognation value can't be NULL
 * @param *slave slave trans , if NULL only set master value , else used for match master
 * @retval false not match
 * @retval true master and slave trans is matched
 */
static bool
mt7915_pdma_match(struct pci_trans *master, struct pci_trans *slave)
{
	u32 master_rid = 0;
	u32 slave_rid = 0;

	if (!master || !slave)
		return false;

	master_rid = get_rid_value(master);
	slave_rid = get_rid_value(slave);

	if (master_rid == DEFAULT_RID)
		set_rid_value(master, master->parent.sid.idx);
	if (slave_rid == DEFAULT_RID)
		set_rid_value(slave, slave->parent.sid.idx);

	return slave_rid == master_rid;
}

static int
mt7915_driver_own(struct mtk_hw_dev *dev, struct mtk_bus_trans *trans, bool drv_own)
{
	struct pci_trans *pci_hw = to_pci_trans(trans);
	u32 reg = mt7915_reg_map_l1(trans, pci_hw->lp_fw_own_reg_addr);

	if (drv_own) {
		bus_write(trans, reg,
		CONN_HOST_CSR_TOP_WF_BAND0_LPCTL_conn_wf_b0_ap_host_clr_fw_own_hs_pulse_MASK);

		if (!mtk_bus_poll_msec(trans, reg,
		CONN_HOST_CSR_TOP_WF_BAND0_LPCTL_conn_wf_b0_host_lpcr_fw_own_sync_MASK,
			    0, 500)) {
			dev_err(trans->pdev, "Timeout for %s\n",
				drv_own ? "driver own" : "fw own");

			return -EIO;
		}
	} else {
		bus_write(trans, reg,
		CONN_HOST_CSR_TOP_WF_BAND0_LPCTL_conn_wf_b0_ap_host_set_fw_own_hs_pulse_MASK);
	}
	return 0;
}

static int
mt7915_check_fwdl_state(struct mtk_hw_dev *dev, u32 state)
{
	struct mtk_bus_trans *trans = dev->bus_trans;
	u32 reg = mt7915_reg_map_l1(trans,
		CONN_HOST_CSR_TOP_CONN_ON_MISC_drv_fw_stat_sync_ADDR);

	dev_info(trans->pdev, "%s(): check target stat=%d, cur state=%d\n",
		__func__, state,
		bus_read(trans, reg) &
		CONN_HOST_CSR_TOP_CONN_ON_MISC_drv_fw_stat_sync_MASK);
	if (!mtk_bus_poll_msec(trans, reg,
			CONN_HOST_CSR_TOP_CONN_ON_MISC_drv_fw_stat_sync_MASK,
			field_prep(CONN_HOST_CSR_TOP_CONN_ON_MISC_drv_fw_stat_sync_MASK,
			   state), 500)) {
		dev_err(trans->pdev,
			"Timeout for initializing firmware, %x\n",
			bus_read(trans,
			CONN_HOST_CSR_TOP_CONN_ON_MISC_drv_fw_stat_sync_ADDR));
		return -EIO;
	}
	return 0;
}

static void
mt7915_set_filter(struct mtk_hw_dev *dev, u8 band, unsigned long flags)
{
	struct mtk_bus_trans *trans = dev->bus_trans;
	u32 filters = bus_read(trans, MT_WF_RFCR(band));

	filters |= MT_WF_RFCR_DROP_FCSFAIL;
	bus_write(trans, MT_WF_RFCR(band), filters);
}

static void
mt7915_mac_mib_update(struct mtk_hw_dev *dev, u8 band_idx, struct mtk_mib_status *mib)
{
	struct mtk_bus_trans *trans = dev->bus_trans;
	int i;

	if (band_idx > MAX_BAND_NUM)
		return;

	memset(mib, 0, sizeof(*mib));

	mib->fcs_err_cnt = bus_get_field(trans, MT_MIB_SDR3(band_idx),
		MT_MIB_SDR3_FCS_ERR_MASK);

	for (i = 0 ; i < 4; i++) {
		u32 val, val2;

		val = bus_read(trans, MT_MIB_MB_SDR1(band_idx, i));

		val2 = field_get(MT_MIB_ACK_FAIL_COUNT_MASK, val);
		if (val2 > mib->ack_fail_cnt)
			mib->ack_fail_cnt = val2;

		val2 = field_get(MT_MIB_BA_MISS_COUNT_MASK, val);
		if (val2 > mib->ba_miss_cnt)
			mib->ba_miss_cnt = val2;

		val = bus_read(trans, MT_MIB_MB_SDR0(band_idx, i));
		val2 = field_get(MT_MIB_RTS_RETRIES_COUNT_MASK, val);
		if (val2 > mib->rts_retries_cnt) {
			mib->rts_cnt = field_get(MT_MIB_RTS_COUNT_MASK, val);
			mib->rts_retries_cnt = val2;
		}
	}
}

static int
mt7915_fwmode_conf(struct mtk_hw_dev *dev)
{
	u32 val;
	struct mtk_bus_trans *trans = dev->bus_trans;

#define SWDEF_MODE_ADDR 0x0041F23C
#define FW_NORMAL_MODE 0
	/*set as normal mode*/
	val = mt7915_l2_rr(trans, SWDEF_MODE_ADDR);
	mt7915_l2_wr(trans, SWDEF_MODE_ADDR, FW_NORMAL_MODE);
	return 0;
}

static void
mt7915_hw_caps_init(struct mtk_hw_dev *dev)
{
	dev->limit |= BIT(LIMIT_SET_BMC_WCID);
	dev->band_idx_bmp_sup = GENMASK(BAND1, BAND0);
	dev->band_num_sup = hweight8(dev->band_idx_bmp_sup);
}

static struct mtk_mcu_dest_2_path mt7915_dest_2_path[MCU_DEST_MAX_NUM] = {
	{MCU_DEST_WM,		MCU_PATH_WA},
	{MCU_DEST_RVD,		MCU_PATH_WA},
	{MCU_DEST_WA,		MCU_PATH_WA},
	{MCU_DEST_WA_WM,	MCU_PATH_WA},
	{MCU_DEST_WO,		MCU_PATH_WO},
};

static int
mt7915_hw_init(struct mtk_hw_dev *dev)
{
	fmac_ops_init(dev);
	mt7915_hw_caps_init(dev);
	return mt7915_fwmode_conf(dev);
}

static struct pci_device_id mt7915_pci_device_table[] = {
	{ PCI_DEVICE(0x14c3, MT7915_CHIP_ID) },
	{ },
};

static struct pci_chip_ops mt7915_pci_ops = {
	.dma_enable = mt7915_pdma_enable,
	.dma_disable = mt7915_pdma_disable,
	.dma_init = mt7915_pdma_init,
	.dma_match = mt7915_pdma_match,
	.dma_schdl_init = NULL,
};

static struct mtk_chip_ctrl_ops mt7915_chip_ops = {
	.driver_own = mt7915_driver_own,
	.check_fwdl_state = mt7915_check_fwdl_state,
	.set_filter = mt7915_set_filter,
	.mac_mib_update = mt7915_mac_mib_update,
	.hw_init = mt7915_hw_init,
};

static struct mtk_hwres_radio_info mt7915_band0[] = {
	{
		.type = HWIFI_PHY_2G,
		.cap = HWIFI_PHY_LEGACY |
			HWIFI_PHY_N |
			HWIFI_PHY_HE
	},
	{
		.type = HWIFI_PHY_5G,
		.cap = HWIFI_PHY_LEGACY |
			HWIFI_PHY_N |
			HWIFI_PHY_VHT |
			HWIFI_PHY_HE
	}
};

static struct mtk_hwres_radio_info mt7915_band1[] = {
	{
		.type = HWIFI_PHY_2G,
		.cap = HWIFI_PHY_LEGACY |
			HWIFI_PHY_N |
			HWIFI_PHY_HE
	},
	{
		.type = HWIFI_PHY_5G,
		.cap = HWIFI_PHY_LEGACY |
			HWIFI_PHY_N |
			HWIFI_PHY_VHT |
			HWIFI_PHY_HE
	}
};

static struct mtk_hwres_radio_cap mt7915_hw_radio_cap[] = {
	{
		/*band_idx 0*/
		.radio_info_size = ARRAY_SIZE(mt7915_band0),
		.info = mt7915_band0,
		.band_idx = 0,
		.wmm_set = {
			.start = 0,
			.end = 1
		},
		.omac = {
			.start = 0,
			.end = 0x4
		},
		.ext_omac = {
			.start = 0x11,
			.end = 0x1f
		},
		.rept_omac = {
			.start = 0x20,
			.end = 0x3f
		},
		.max_tx_tk_nums = 8192,
	},
	{
		/*band_idx 1*/
		.radio_info_size = ARRAY_SIZE(mt7915_band1),
		.info = mt7915_band1,
		.band_idx = 1,
		.wmm_set = {
			.start = 2,
			.end = 3
		},
		.omac = {
			.start = 0,
			.end = 0x4
		},
		.ext_omac = {
			.start = 0x11,
			.end = 0x1f
		},
		.rept_omac = {
			.start = 0x20,
			.end = 0x3f
		},
		.max_tx_tk_nums = 8192,
	}
};

static struct mtk_hwres_cap mt7915_hw_res = {
	.uwtbl = {
		.start = 1,
		.end = 219,
	},
	.group = {
		.start = 220,
		.end = 255,
	},
	.tx_token = {
		.start = 0,
		.end = 8191,
	},
	.rx_token = {
		.start = 0,
		.end = 7167,
	},
	.bss = {
		.start = 0,
		.end = 31,
	},
	.mld_addr = {
		.start = 0,
		.end = 15,
	},
	.link_addr = {
		.start = 16,
		.end = 63
	},
	.mld_remap = {
		.start = 0,
		.end = 15,
	},
	.radio_num = ARRAY_SIZE(mt7915_hw_radio_cap),
	.radio_cap = mt7915_hw_radio_cap
};

static struct mtk_chip_hw_cap mt7915_chip_cap = {
	.dev_size = sizeof(struct mtk_hw_dev),
	.fw_flags = BIT(FW_FLAG_WM) | BIT(FW_FLAG_WA),
	.mac_type = MAC_TYPE_FMAC,
	.eeprom_default = MT7915_E2PImage_ePAeLNA,
	.eeprom_size = sizeof(MT7915_E2PImage_ePAeLNA),
	.mtxd_sz = 148,
	.mrxd_sz = 0x60,
	.mac_rxd_grp_0_sz = 6 * 4,
	.hwres = &mt7915_hw_res,
	.mcu_infos = {
		[MCU_WM] = {
			.fw = {
				[MCU_FW_MODE_NORMALMODE] = MT7915_FIRMWARE_WM,
				[MCU_FW_MODE_TESTMODE] = MT7915_FIRMWARE_WM,
			},
			.fw_mode = MCU_FW_MODE_NORMALMODE,
			.rom_patch = MT7915_ROM_PATCH,
			.opt = BIT(WM_OPT_PATCH_DL) | BIT(WM_OPT_UNICMD_NOT_SUPP)
		},
		[MCU_WA] = {
			.fw = {
				[MCU_FW_MODE_NORMALMODE] = MT7915_FIRMWARE_WA,
				[MCU_FW_MODE_TESTMODE] = NULL,
			},
			.fw_mode = MCU_FW_MODE_NORMALMODE,
			.rom_patch = NULL,
			.opt = 0
		}
	},
	.mac_cap = BIT(CAP_OFFLOAD_TXD),
};

static struct mtk_chip_drv mt7915_drv = {
	.bus_cfg = {
		.bus_type = MTK_BUS_PCI,
		.bus_sub_type = MTK_BUS_NUM,
		.id_table = mt7915_pci_device_table,
		.name = KBUILD_MODNAME,
		.profile = &mt7915_pci_profile,
		.ms_type = CHIP_TYPE_MASTER,
		.bus_ops = &mt7915_pci_ops,
		.bus_drv_sz = sizeof(struct pci_driver),
	},
	.ctl_ops = &mt7915_chip_ops,
	.hw_caps = &mt7915_chip_cap,
	.device_id = MT7915_CHIP_ID,
	.dest_2_path = mt7915_dest_2_path,
};

static int __init mtk_chip_7915_init(void)
{
	/*insert module with mac_type can dynamic change*/
	mt7915_drv.interface_type = mac_type;
#ifdef CONFIG_HWIFI_HIF_WED
	mt7915_drv.bus_cfg.bus_type = MTK_BUS_WED;
	mt7915_drv.bus_cfg.bus_sub_type = MTK_BUS_PCI;
	mt7915_drv.bus_cfg.bus_drv_sz = sizeof(struct wed_driver);
	mt7915_drv.bus_cfg.profile = &mt7915_pci_wed_profile;
	mt7915_hw_res.tx_token.start = 8192;
	mt7915_hw_res.tx_token.end = 9215;
	mt7915_hw_res.ext_tx_tk_nums_master = 8192; /* TKID num */
	mt7915_hw_res.ext_tx_tk_nums_slave = 8192;
	mt7915_hw_res.ext_rx_tk_nums_master = 8192; /* RXBM buffer num */
	mt7915_hw_res.ext_rx_tk_nums_slave = 8192;
	mt7915_hw_res.ext_tx_buf_nums_master = 8192; /* TXBM buffer num */
	mt7915_hw_res.ext_tx_buf_nums_slave = 8192;
	mt7915_hw_radio_cap[0].max_tx_tk_nums = 1024;
	mt7915_hw_radio_cap[1].max_tx_tk_nums = 1024;

	mt7915_hw_res.rx_token.end += mt7915_hw_res.ext_rx_tk_nums_master +
				      mt7915_hw_res.ext_rx_tk_nums_slave;
#endif
	return mtk_chip_register(&mt7915_drv);
}

static void __exit mtk_chip_7915_exit(void)
{
	mtk_chip_unregister(&mt7915_drv);
}

module_init(mtk_chip_7915_init);
module_exit(mtk_chip_7915_exit);

MODULE_LICENSE("Dual BSD/GPL");
MODULE_DESCRIPTION("Mediatek Hardware Wi-Fi Module");
