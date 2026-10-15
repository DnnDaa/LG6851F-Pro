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

static void
_mt7916_pdma_irq_init(struct pci_trans *pci_hw)
{
	struct mtk_bus_trans *trans = to_bus_trans(pci_hw);

	bus_set(trans, WF_WFDMA_HOST_DMA0_PCIE1_WPDMA_INT_RX_PRI_SEL_ADDR,
		WF_WFDMA_HOST_DMA0_PCIE1_WPDMA_INT_RX_PRI_SEL_WPDMA_INT_RX_RING1_PRI_SEL_MASK);
	bus_set(trans, WF_WFDMA_HOST_DMA1_PCIE1_WPDMA_INT_RX_PRI_SEL_ADDR,
		WF_WFDMA_HOST_DMA1_PCIE1_WPDMA_INT_RX_PRI_SEL_WPDMA_INT_RX_RING2_PRI_SEL_MASK);
	bus_set(trans, WF_WFDMA_HOST_DMA1_PCIE1_WPDMA_INT_TX_PRI_SEL_ADDR,
		WF_WFDMA_HOST_DMA1_PCIE1_WPDMA_INT_TX_PRI_SEL_WPDMA_INT_TX_RING19_PRI_SEL_MASK);
}

static void
_mt7916_irq_disable(struct pci_trans *pci_hw)
{
	struct mtk_bus_trans *trans = to_bus_trans(pci_hw);
	unsigned long flags;
	u32 mac_int_addr = MT_PCIE_MAC1_INT_ENABLE_ADDR;

	spin_lock_irqsave(&pci_hw->irq_lock, flags);
	bus_write(trans, pci_hw->int_mask_map[0].int_ena_addr, 0);
	mt7915_l1_wr(trans, mac_int_addr, 0x0);
	spin_unlock_irqrestore(&pci_hw->irq_lock, flags);
	clear_bit(PCI_STATE_ISR_START, &pci_hw->state);
}

static int
_mt7916_irq_enable(struct pci_trans *pci_hw)
{
	struct mtk_bus_trans *trans = to_bus_trans(pci_hw);
	unsigned long flags;
	u32 mac_int_addr = MT_PCIE_MAC1_INT_ENABLE_ADDR;

	spin_lock_irqsave(&pci_hw->irq_lock, flags);

	bus_write(trans, pci_hw->int_mask_map[0].int_sts_addr, 0xffffffff);
	bus_write(trans, pci_hw->int_mask_map[0].int_ena_addr, 0);
	/* clear garbage ints */
	bus_write(trans, pci_hw->int_mask_map[0].int_ena_addr,
			 pci_hw->int_mask_map[0].int_ena_mask);
	/* Master Switch of PCIE Interrupt Enable */
	mt7915_l1_wr(trans, mac_int_addr, 0x000000ff);
	spin_unlock_irqrestore(&pci_hw->irq_lock, flags);

	set_bit(PCI_STATE_ISR_START, &pci_hw->state);
	return 0;
}

static void
_mt7916_pdma_init(struct pci_trans *pci_hw)
{
	struct mtk_bus_trans *trans = to_bus_trans(pci_hw);

	/* configure global setting */
	/* host DMA1(to WMCPU/WACPU) need to omit tx/rx info */
	bus_set(trans, WF_WFDMA_HOST_DMA1_PCIE1_WPDMA_GLO_CFG_ADDR,
		WF_WFDMA_HOST_DMA1_PCIE1_WPDMA_GLO_CFG_OMIT_TX_INFO_MASK |
		WF_WFDMA_HOST_DMA1_PCIE1_WPDMA_GLO_CFG_OMIT_RX_INFO_MASK |
		WF_WFDMA_HOST_DMA1_PCIE1_WPDMA_GLO_CFG_PDMA_ADDR_EXT_EN_SHFT);

	/* configure prefetch setting */
	if (test_bit(PCI_FLAG_PREFETCH_MANUAL, &pci_hw->flags)) {
		/* disable prefetch offset calculation auto-mode */
		bus_clear(trans, WF_WFDMA_HOST_DMA0_PCIE1_WPDMA_GLO_CFG_ADDR,
			WF_WFDMA_HOST_DMA0_PCIE1_WPDMA_GLO_CFG_CSR_DISP_BASE_PTR_CHAIN_EN_MASK);

		/* disable prefetch offset calculation auto-mode */
		bus_clear(trans, WF_WFDMA_HOST_DMA1_PCIE1_WPDMA_GLO_CFG_ADDR,
			WF_WFDMA_HOST_DMA1_PCIE1_WPDMA_GLO_CFG_CSR_DISP_BASE_PTR_CHAIN_EN_MASK);
	}

	bus_write(trans, WF_WFDMA_HOST_DMA0_PCIE1_WPDMA_RX_RING1_EXT_CTRL_ADDR, 0x04800004);
	bus_write(trans, WF_WFDMA_HOST_DMA0_PCIE1_WPDMA_RX_RING2_EXT_CTRL_ADDR, 0x04c00000);

	bus_write(trans, WF_WFDMA_HOST_DMA1_PCIE1_WPDMA_TX_RING19_EXT_CTRL_ADDR, 0x04c00004);
	bus_write(trans, (WF_WFDMA_HOST_DMA1_PCIE1_BASE + 0x610), 0x05000000);

	bus_write(trans, WF_WFDMA_HOST_DMA1_PCIE1_WPDMA_RX_RING2_EXT_CTRL_ADDR, 0x05000004);
	bus_write(trans, WF_WFDMA_HOST_DMA1_PCIE1_WPDMA_RX_RING3_EXT_CTRL_ADDR, 0x05400000);

	/* reset dma idx */
	bus_write(trans, WF_WFDMA_HOST_DMA0_PCIE1_WPDMA_RST_DTX_PTR_ADDR, 0xffffffff);
	bus_write(trans, WF_WFDMA_HOST_DMA1_PCIE1_WPDMA_RST_DTX_PTR_ADDR, 0xffffffff);

	/* enable PCIe 1 TXP cut through */
	bus_set(trans, WF_WFDMA_EXT_WRAP_CSR_WFDMA_HOST_CONFIG_pdma_per_band_ADDR,
			WF_WFDMA_EXT_WRAP_CSR_WFDMA_HOST_CONFIG_pdma_per_band_MASK);
}

static bool
_mt7916_pdma_poll(struct pci_trans *pci_hw, u32 round, u32 wait_us)
{
	struct mtk_bus_trans *trans = to_bus_trans(pci_hw);
	u8 i = 0;
	u32 val = 0;
	bool idle;

	bus_set(trans, WF_WFDMA_HOST_DMA0_CONN_HIF_BUSY_ENA_ADDR,
		WF_WFDMA_HOST_DMA0_CONN_HIF_BUSY_ENA_conn_hif_rxfifo_busy_enable_MASK |
		WF_WFDMA_HOST_DMA0_CONN_HIF_BUSY_ENA_conn_hif_txfifo0_busy_enable_MASK |
		WF_WFDMA_HOST_DMA0_CONN_HIF_BUSY_ENA_conn_hif_txfifo1_busy_enable_MASK);

	bus_set(trans, WF_WFDMA_HOST_DMA1_CONN_HIF_BUSY_ENA_ADDR,
		WF_WFDMA_HOST_DMA1_CONN_HIF_BUSY_ENA_conn_hif_rxfifo_busy_enable_MASK |
		WF_WFDMA_HOST_DMA1_CONN_HIF_BUSY_ENA_conn_hif_txfifo0_busy_enable_MASK |
		WF_WFDMA_HOST_DMA1_CONN_HIF_BUSY_ENA_conn_hif_txfifo1_busy_enable_MASK);

	val = bus_read(trans, WF_WFDMA_HOST_DMA0_PCIE1_CONN_HIF_BUSY_ENA_ADDR);
	val |= WF_WFDMA_HOST_DMA0_PCIE1_CONN_HIF_BUSY_ENA_conn_hif_rxfifo_busy_enable_MASK;
	val &= ~(WF_WFDMA_HOST_DMA0_PCIE1_CONN_HIF_BUSY_ENA_conn_hif_txfifo0_busy_enable_MASK |
		WF_WFDMA_HOST_DMA0_PCIE1_CONN_HIF_BUSY_ENA_conn_hif_txfifo1_busy_enable_MASK);
	bus_write(trans, WF_WFDMA_HOST_DMA0_PCIE1_CONN_HIF_BUSY_ENA_ADDR, val);

	bus_set(trans, WF_WFDMA_HOST_DMA1_PCIE1_CONN_HIF_BUSY_ENA_ADDR,
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
_mt7916_pdma_reset(struct pci_trans *pci_hw)
{
	struct mtk_bus_trans *trans = to_bus_trans(pci_hw);

	bus_clear(trans, WF_WFDMA_HOST_DMA1_PCIE1_CONN_HIF_RST_ADDR,
		WF_WFDMA_HOST_DMA1_PCIE1_CONN_HIF_RST_conn_hif_logic_rst_n_MASK |
		WF_WFDMA_HOST_DMA1_PCIE1_CONN_HIF_RST_dmashdl_all_rst_n_MASK);

	bus_set(trans, WF_WFDMA_HOST_DMA1_PCIE1_CONN_HIF_RST_ADDR,
		WF_WFDMA_HOST_DMA1_PCIE1_CONN_HIF_RST_conn_hif_logic_rst_n_MASK |
		WF_WFDMA_HOST_DMA1_PCIE1_CONN_HIF_RST_dmashdl_all_rst_n_MASK);

	bus_clear(trans, WF_WFDMA_HOST_DMA0_PCIE1_CONN_HIF_RST_ADDR,
		WF_WFDMA_HOST_DMA0_PCIE1_CONN_HIF_RST_conn_hif_logic_rst_n_MASK |
		WF_WFDMA_HOST_DMA0_PCIE1_CONN_HIF_RST_dmashdl_all_rst_n_MASK);

	bus_set(trans, WF_WFDMA_HOST_DMA0_PCIE1_CONN_HIF_RST_ADDR,
		WF_WFDMA_HOST_DMA0_PCIE1_CONN_HIF_RST_conn_hif_logic_rst_n_MASK |
		WF_WFDMA_HOST_DMA0_PCIE1_CONN_HIF_RST_dmashdl_all_rst_n_MASK);
}

static int
_mt7916_pdma_disable(struct pci_trans *pci_hw)
{
	struct mtk_bus_trans *trans = to_bus_trans(pci_hw);

	_mt7916_irq_disable(pci_hw);
	bus_clear(trans, WF_WFDMA_HOST_DMA0_PCIE1_WPDMA_GLO_CFG_ADDR,
			WF_WFDMA_HOST_DMA0_PCIE1_WPDMA_GLO_CFG_TX_DMA_EN_MASK |
			WF_WFDMA_HOST_DMA0_PCIE1_WPDMA_GLO_CFG_RX_DMA_EN_MASK);

	bus_clear(trans, WF_WFDMA_HOST_DMA1_PCIE1_WPDMA_GLO_CFG_ADDR,
			WF_WFDMA_HOST_DMA1_PCIE1_WPDMA_GLO_CFG_TX_DMA_EN_MASK |
			WF_WFDMA_HOST_DMA0_PCIE1_WPDMA_GLO_CFG_RX_DMA_EN_MASK);
	return 0;
}

static int
_mt7916_pdma_enable(struct pci_trans *pci_hw)
{
	struct mtk_bus_trans *trans = to_bus_trans(pci_hw);

	/*configure two port pcie configuration*/
	bus_set(trans, WF_WFDMA_HOST_DMA0_PCIE1_WPDMA_GLO_CFG_ADDR,
		WF_WFDMA_HOST_DMA0_PCIE1_WPDMA_GLO_CFG_TX_DMA_EN_MASK |
		WF_WFDMA_HOST_DMA0_PCIE1_WPDMA_GLO_CFG_RX_DMA_EN_MASK);

	bus_set(trans, WF_WFDMA_HOST_DMA1_PCIE1_WPDMA_GLO_CFG_ADDR,
		WF_WFDMA_HOST_DMA1_PCIE1_WPDMA_GLO_CFG_TX_DMA_EN_MASK |
		WF_WFDMA_HOST_DMA1_PCIE1_WPDMA_GLO_CFG_RX_DMA_EN_MASK);
	return 0;
}

static int
mt7916_pdma_enable(struct pci_trans *trans)
{
	int ret = -EINVAL;

#define PDMA_POLL_ROUND 100
#define PDMA_POLL_US 1000

	if (!test_bit(PCI_FLAG_TWO_PCIE, &trans->flags))
		goto err;

	/*two port pcie recognation*/
	if (get_rid_value(trans) == DEFAULT_RID)
		goto err;

	ret = _mt7916_pdma_poll(trans, PDMA_POLL_ROUND, PDMA_POLL_US);
	if (ret)
		goto err;
	ret = _mt7916_pdma_enable(trans);
err:
	return ret;
}

static int
mt7916_pdma_disable(struct pci_trans *trans)
{
	if (!test_bit(PCI_FLAG_TWO_PCIE, &trans->flags))
		goto err;

	/*two port pcie recognation*/
	if (get_rid_value(trans) == DEFAULT_RID)
		goto err;

	_mt7916_pdma_reset(trans);
	_mt7916_pdma_disable(trans);
	return 0;
err:
	return -EPERM;
}

static int
mt7916_pdma_init(struct pci_trans *trans)
{

	if (!test_bit(PCI_FLAG_TWO_PCIE, &trans->flags))
		goto err;

	/*two port pcie recognation*/
	if (get_rid_value(trans) == DEFAULT_RID)
		goto err;

	_mt7916_pdma_init(trans);
	_mt7916_pdma_irq_init(trans);
	_mt7916_irq_enable(trans);
	return 0;
err:
	return -EPERM;
}

static struct mtk_mcu_dest_2_path mt7916_dest_2_path[MCU_DEST_MAX_NUM] = {
	{MCU_DEST_WM,		MCU_PATH_WA},
	{MCU_DEST_RVD,		MCU_PATH_WA},
	{MCU_DEST_WA,		MCU_PATH_WA},
	{MCU_DEST_WA_WM,	MCU_PATH_WA},
	{MCU_DEST_WO,		MCU_PATH_WO},
};


/**
 * @brief chip specific recognation machanism
 *
 * @param *master master trans used to set recognation value can't be NULL
 * @param *slave slave trans , if NULL only set master value , else used for match master
 * @retval false not match
 * @retval true master and slave trans is matched
 */
static bool
mt7916_pdma_match(struct pci_trans *master, struct pci_trans *slave)
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

/*mt7916 is slave pcie port, only pci ops is necessary*/
struct pci_chip_ops mt7916_pci_ops = {
	.dma_enable = mt7916_pdma_enable,
	.dma_disable = mt7916_pdma_disable,
	.dma_init = mt7916_pdma_init,
	.dma_match = mt7916_pdma_match,
	.dma_schdl_init = NULL,
};


static struct pci_device_id mt7916_pci_device_table[] = {
	{ PCI_DEVICE(0x14c3, 0x7916) },
	{ },
};

static struct mtk_chip_drv mt7916_drv = {
	.bus_cfg = {
		.bus_type = MTK_BUS_PCI,
		.id_table = mt7916_pci_device_table,
		.name = KBUILD_MODNAME,
		.profile = &mt7916_pci_profile,
		.ms_type = CHIP_TYPE_SLAVE,
		.bus_ops = &mt7916_pci_ops,
		.bus_drv_sz = sizeof(struct pci_driver),
	},
	.device_id = 0x7916,
	.interface_type = MTK_INTERFACE_MAC80211,
	.dest_2_path = mt7916_dest_2_path,
};


static int __init mtk_chip_7916_init(void)
{
	return mtk_chip_register(&mt7916_drv);
}

static void __exit mtk_chip_7916_exit(void)
{
	mtk_chip_unregister(&mt7916_drv);
}


module_init(mtk_chip_7916_init);
module_exit(mtk_chip_7916_exit);


MODULE_LICENSE("Dual BSD/GPL");
MODULE_DESCRIPTION("Mediatek Hardware Wi-Fi Module");

