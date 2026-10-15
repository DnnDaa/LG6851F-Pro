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
#include <linux/firmware.h>
#include "bellwether.h"
#include "mac_if.h"
#include "core.h"
#include "chips/bmac.h"

#include "conn_infra_bus_cr_on.h"
#include "wf_mcu_bus_cr.h"
#include "conn_infra_rgu_on.h"
#include "wf_top_cfg_on.h"
#include "conn_host_csr_top.h"

//#include "firmware/bellwether_e2p_ePAeLNA.h"

/*TBD: not ready*/
static int mac_type = MTK_INTERFACE_CONNAC;
module_param(mac_type, int, 0644);
MODULE_PARM_DESC(mac_type, "mac driver type");

#define BELLWETHER_CHIP_ID 0x7902

static void
dump_chip_info(struct pci_trans *pci_hw)
{
	struct mtk_bus_trans *trans = to_bus_trans(pci_hw);
	u32 chipid = BELLWETHER_CHIP_ID;

	dev_info(to_device(trans),
		"%s(): chip id: %x\n",
		__func__, chipid);
}

static void
_bellwether_pdma_irq_clear_int_sta(struct pci_trans *pci_hw)
{
	struct mtk_bus_trans *trans = to_bus_trans(pci_hw);
	unsigned long flags;

	spin_lock_irqsave(&pci_hw->irq_lock, flags);
	bus_write(trans, pci_hw->mcu2host_sw_int_sta_addr, 0xffffffff);
	bus_write(trans, pci_hw->int_mask_map[0].int_sts_addr, 0xffffffff);
	spin_unlock_irqrestore(&pci_hw->irq_lock, flags);
}

static void
_bellwether_pdma_irq_init(struct pci_trans *pci_hw)
{
	_bellwether_pdma_irq_clear_int_sta(pci_hw);
}

static void
_bellwether_pdma_irq_disable(struct pci_trans *pci_hw)
{
	struct mtk_bus_trans *trans = to_bus_trans(pci_hw);
	unsigned long flags;

	spin_lock_irqsave(&pci_hw->irq_lock, flags);
	bus_write(trans, pci_hw->mcu2host_sw_int_ena_addr, 0);
	bus_write(trans, pci_hw->int_mask_map[0].int_ena_addr, 0);
	bus_write(trans, MT_PCIE_MAC_INT_ENABLE_ADDR, 0x0);
	spin_unlock_irqrestore(&pci_hw->irq_lock, flags);
	clear_bit(PCI_STATE_ISR_START, &pci_hw->state);
}

static void
_bellwether_pdma_irq_enable(struct pci_trans *pci_hw)
{
	struct mtk_bus_trans *trans = to_bus_trans(pci_hw);
	unsigned long flags;

	spin_lock_irqsave(&pci_hw->irq_lock, flags);
	/* Master Switch of PCIE Interrupt Enable */
	bus_write(trans, MT_PCIE_MAC_INT_ENABLE_ADDR, 0x000000ff);
	bus_write(trans, pci_hw->int_mask_map[0].int_ena_addr,
			 pci_hw->int_mask_map[0].int_ena_mask);
	bus_write(trans, pci_hw->mcu2host_sw_int_ena_addr,
		pci_hw->mcu2host_sw_int_ser_mask |
		pci_hw->mcu2host_sw_wdt_ser_mask);
	spin_unlock_irqrestore(&pci_hw->irq_lock, flags);
	clear_bit(PCI_STATE_ISR_PEDING, &pci_hw->state);
	set_bit(PCI_STATE_ISR_START, &pci_hw->state);
}

static void
_bellwether_pdma_config(struct pci_trans *pci_hw)
{
	struct mtk_bus_trans *trans = to_bus_trans(pci_hw);

	/* GLO_CFG */
	bus_set(trans, WF_WFDMA_HOST_DMA0_WPDMA_GLO_CFG_ADDR,
		WF_WFDMA_HOST_DMA0_WPDMA_GLO_CFG_OMIT_TX_INFO_MASK |
		WF_WFDMA_HOST_DMA0_WPDMA_GLO_CFG_OMIT_RX_INFO_PFET2_MASK |
		WF_WFDMA_HOST_DMA0_WPDMA_GLO_CFG_PDMA_ADDR_EXT_EN_MASK);

	/* GLO_CFG_EXT0 */
	bus_set(trans, WF_WFDMA_HOST_DMA0_WPDMA_GLO_CFG_EXT0_ADDR,
		WF_WFDMA_HOST_DMA0_WPDMA_GLO_CFG_EXT0_CSR_RX_WB_KEEP_RSVD_MASK);

	/* GLO_CFG_EXT1 */
	bus_set(trans, WF_WFDMA_HOST_DMA0_WPDMA_GLO_CFG_EXT1_ADDR,
		WF_WFDMA_HOST_DMA0_WPDMA_GLO_CFG_EXT1_CSR_TX_FCTRL_MODE_MASK);

	/*enable buffer ready bypass for tput*/
	bus_set(trans, WF_WFDMA_EXT_WRAP_CSR_WFDMA_AXI0_R2A_CTRL_0_BUFRDY_BYP_ADDR,
		WF_WFDMA_EXT_WRAP_CSR_WFDMA_AXI0_R2A_CTRL_0_BUFRDY_BYP_MASK);
}

static void
_bellwether_pdma_prefetch_config(struct pci_trans *pci_hw)
{
	struct mtk_bus_trans *trans = to_bus_trans(pci_hw);

	/* Rx */
	bus_write(trans, WF_WFDMA_HOST_DMA0_WPDMA_RX_RING0_EXT_CTRL_ADDR, 0x00000004);
	bus_write(trans, WF_WFDMA_HOST_DMA0_WPDMA_RX_RING1_EXT_CTRL_ADDR, 0x00400004);
	bus_write(trans, WF_WFDMA_HOST_DMA0_WPDMA_RX_RING2_EXT_CTRL_ADDR, 0x00800004);
	bus_write(trans, WF_WFDMA_HOST_DMA0_WPDMA_RX_RING3_EXT_CTRL_ADDR, 0x00c00000);
	bus_write(trans, WF_WFDMA_HOST_DMA0_WPDMA_RX_RING4_EXT_CTRL_ADDR, 0x00c00010);
	bus_write(trans, WF_WFDMA_HOST_DMA0_WPDMA_RX_RING5_EXT_CTRL_ADDR, 0x01c00010);
	bus_write(trans, WF_WFDMA_HOST_DMA0_WPDMA_RX_RING6_EXT_CTRL_ADDR, 0x02c00004);
	bus_write(trans, WF_WFDMA_HOST_DMA0_WPDMA_RX_RING7_EXT_CTRL_ADDR, 0x03000004);
	bus_write(trans, WF_WFDMA_HOST_DMA0_WPDMA_RX_RING8_EXT_CTRL_ADDR, 0x03400010);
	bus_write(trans, WF_WFDMA_HOST_DMA0_WPDMA_RX_RING9_EXT_CTRL_ADDR, 0x04400004);

	/* Tx */
	bus_write(trans, WF_WFDMA_HOST_DMA0_WPDMA_TX_RING0_EXT_CTRL_ADDR, 0x04800004);
	bus_write(trans, WF_WFDMA_HOST_DMA0_WPDMA_TX_RING1_EXT_CTRL_ADDR, 0x04c00004);
	bus_write(trans, WF_WFDMA_HOST_DMA0_WPDMA_TX_RING2_EXT_CTRL_ADDR, 0x05000004);
	bus_write(trans, WF_WFDMA_HOST_DMA0_WPDMA_TX_RING3_EXT_CTRL_ADDR, 0x05400000);
	bus_write(trans, WF_WFDMA_HOST_DMA0_WPDMA_TX_RING4_EXT_CTRL_ADDR, 0x05400000);
	bus_write(trans, WF_WFDMA_HOST_DMA0_WPDMA_TX_RING5_EXT_CTRL_ADDR, 0x05400000);
	bus_write(trans, WF_WFDMA_HOST_DMA0_WPDMA_TX_RING6_EXT_CTRL_ADDR, 0x05400000);
	bus_write(trans, WF_WFDMA_HOST_DMA0_WPDMA_TX_RING16_EXT_CTRL_ADDR, 0x05400004);
	bus_write(trans, WF_WFDMA_HOST_DMA0_WPDMA_TX_RING17_EXT_CTRL_ADDR, 0x05800004);
	bus_write(trans, WF_WFDMA_HOST_DMA0_WPDMA_TX_RING18_EXT_CTRL_ADDR, 0x05c00004);
	bus_write(trans, WF_WFDMA_HOST_DMA0_WPDMA_TX_RING19_EXT_CTRL_ADDR, 0x06000004);
	bus_write(trans, WF_WFDMA_HOST_DMA0_WPDMA_TX_RING20_EXT_CTRL_ADDR, 0x06400004);
	bus_write(trans, WF_WFDMA_HOST_DMA0_WPDMA_TX_RING21_EXT_CTRL_ADDR, 0x06800004);
}

static void
_bellwether_pdma_reset_didx(struct pci_trans *pci_hw)
{
	struct mtk_bus_trans *trans = to_bus_trans(pci_hw);

	bus_write(trans, WF_WFDMA_HOST_DMA0_WPDMA_RST_DTX_PTR_ADDR, 0xffffffff);
	bus_write(trans, WF_WFDMA_HOST_DMA0_WPDMA_RST_DRX_PTR_ADDR, 0xffffffff);
}

static void
_bellwether_pdma_init(struct pci_trans *pci_hw)
{
	_bellwether_pdma_config(pci_hw);
	_bellwether_pdma_prefetch_config(pci_hw);
	_bellwether_pdma_reset_didx(pci_hw);
}

static void
_bellwether_pdma_unpause(struct pci_trans *pci_hw)
{
	bus_write(to_bus_trans(pci_hw), pci_hw->pause_trxq_reg_addr, 0);
}

static void
_bellwether_pdma_pause(struct pci_trans *pci_hw)
{
}

static bool
_bellwether_pdma_poll(struct pci_trans *pci_hw, u32 round, u32 wait_us)
{
	struct mtk_bus_trans *trans = to_bus_trans(pci_hw);
	u32 val;
	u8 i = 0;
	bool idle = false;

	bus_set(trans, WF_WFDMA_HOST_DMA0_CONN_HIF_BUSY_ENA_ADDR,
		WF_WFDMA_HOST_DMA0_CONN_HIF_BUSY_ENA_conn_hif_rxfifo_busy_enable_MASK |
		WF_WFDMA_HOST_DMA0_CONN_HIF_BUSY_ENA_conn_hif_txfifo0_busy_enable_MASK |
		WF_WFDMA_HOST_DMA0_CONN_HIF_BUSY_ENA_conn_hif_txfifo1_busy_enable_MASK);

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
_bellwether_pdma_reset(struct pci_trans *pci_hw)
{
	struct mtk_bus_trans *trans = to_bus_trans(pci_hw);

	bus_clear(trans, WF_WFDMA_HOST_DMA0_CONN_HIF_RST_ADDR,
		WF_WFDMA_HOST_DMA0_CONN_HIF_RST_conn_hif_logic_rst_n_MASK |
		WF_WFDMA_HOST_DMA0_CONN_HIF_RST_dmashdl_all_rst_n_MASK);

	bus_set(trans, WF_WFDMA_HOST_DMA0_CONN_HIF_RST_ADDR,
		WF_WFDMA_HOST_DMA0_CONN_HIF_RST_conn_hif_logic_rst_n_MASK |
		WF_WFDMA_HOST_DMA0_CONN_HIF_RST_dmashdl_all_rst_n_MASK);
}

static void
_bellwether_pdma_enable(struct pci_trans *pci_hw)
{
	struct mtk_bus_trans *trans = to_bus_trans(pci_hw);

	if (test_bit(PCI_FLAG_WHNAT_RX, &pci_hw->flags))
		bus_set(trans, WF_WFDMA_HOST_DMA0_WPDMA_GLO_CFG_ADDR,
			WF_WFDMA_HOST_DMA0_WPDMA_GLO_CFG_TX_DMA_EN_MASK);
	else
		bus_set(trans, WF_WFDMA_HOST_DMA0_WPDMA_GLO_CFG_ADDR,
			WF_WFDMA_HOST_DMA0_WPDMA_GLO_CFG_TX_DMA_EN_MASK |
			WF_WFDMA_HOST_DMA0_WPDMA_GLO_CFG_RX_DMA_EN_MASK);
}

static void
_bellwether_pdma_disable(struct pci_trans *pci_hw)
{
	struct mtk_bus_trans *trans = to_bus_trans(pci_hw);

	bus_clear(trans, WF_WFDMA_HOST_DMA0_WPDMA_GLO_CFG_ADDR,
			WF_WFDMA_HOST_DMA0_WPDMA_GLO_CFG_TX_DMA_EN_MASK |
			WF_WFDMA_HOST_DMA0_WPDMA_GLO_CFG_RX_DMA_EN_MASK);
}

static int
bellwether_pdma_enable(struct pci_trans *trans)
{
	_bellwether_pdma_irq_enable(trans);
	_bellwether_pdma_unpause(trans);
	_bellwether_pdma_enable(trans);

	return 0;
}

static int
bellwether_pdma_disable(struct pci_trans *trans)
{
#define PDMA_POLL_ROUND 100
#define PDMA_POLL_US 1000
	int ret;

	_bellwether_pdma_disable(trans);
	_bellwether_pdma_pause(trans);
	ret = _bellwether_pdma_poll(trans, PDMA_POLL_ROUND, PDMA_POLL_US);
	if (ret)
		dev_err(to_device(trans), "%s(): Polling to idle failed\n", __func__);

	_bellwether_pdma_irq_disable(trans);
	_bellwether_pdma_reset(trans);

	return ret;
}

static int
bellwether_pdma_init(struct pci_trans *trans)
{
	dump_chip_info(trans);
	_bellwether_pdma_init(trans);
	_bellwether_pdma_irq_init(trans);

	return 0;
}

#ifdef CONFIG_HWIFI_MOBILE_SUPPORT
static void
_wfdma_schdl_set_ple_max_page(struct mtk_bus_trans *trans, u16 max_page)
{
	u32 value = 0;

	value = bus_read(trans, WF_HIF_DMASHDL_TOP_PACKET_MAX_SIZE_ADDR);

	value &= ~WF_HIF_DMASHDL_TOP_PACKET_MAX_SIZE_PLE_PACKET_MAX_SIZE_MASK;
	value |= (max_page <<
		  WF_HIF_DMASHDL_TOP_PACKET_MAX_SIZE_PLE_PACKET_MAX_SIZE_SHFT) &
		 WF_HIF_DMASHDL_TOP_PACKET_MAX_SIZE_PLE_PACKET_MAX_SIZE_MASK;

	bus_write(trans, WF_HIF_DMASHDL_TOP_PACKET_MAX_SIZE_ADDR, value);
}

static void
_wfdma_schdl_set_pse_max_page(struct mtk_bus_trans *trans, u16 max_page)
{
	u32 value = 0;

	value = bus_read(trans, WF_HIF_DMASHDL_TOP_PACKET_MAX_SIZE_ADDR);

	value &= ~WF_HIF_DMASHDL_TOP_PACKET_MAX_SIZE_PSE_PACKET_MAX_SIZE_MASK;
	value |= (max_page <<
		  WF_HIF_DMASHDL_TOP_PACKET_MAX_SIZE_PSE_PACKET_MAX_SIZE_SHFT) &
		 WF_HIF_DMASHDL_TOP_PACKET_MAX_SIZE_PSE_PACKET_MAX_SIZE_MASK;

	bus_write(trans, WF_HIF_DMASHDL_TOP_PACKET_MAX_SIZE_ADDR, value);
}

static void
_wfdma_schdl_set_refill(struct mtk_bus_trans *trans, u8 grp, bool enable)
{
	u32 mask;
	u32 value = 0;

	mask = WF_HIF_DMASHDL_TOP_REFILL_CONTROL_GROUP0_REFILL_DISABLE_MASK << grp;

	value = bus_read(trans, WF_HIF_DMASHDL_TOP_REFILL_CONTROL_ADDR);

	if (enable)
		value &= ~mask;
	else
		value |= mask;

	bus_write(trans, WF_HIF_DMASHDL_TOP_REFILL_CONTROL_ADDR, value);
}

static void
_wfdma_schdl_set_max_quota(struct mtk_bus_trans *trans, u8 group, u16 quota)
{
	u32 addr;
	u32 value = 0;

	addr = WF_HIF_DMASHDL_TOP_GROUP0_CONTROL_ADDR + (group << 2);

	value = bus_read(trans, addr);

	value &= ~WF_HIF_DMASHDL_TOP_GROUP0_CONTROL_GROUP0_MAX_QUOTA_MASK;
	value |= (quota <<
		  WF_HIF_DMASHDL_TOP_GROUP0_CONTROL_GROUP0_MAX_QUOTA_SHFT) &
		 WF_HIF_DMASHDL_TOP_GROUP0_CONTROL_GROUP0_MAX_QUOTA_MASK;

	bus_write(trans, addr, value);
}

static void
_wfdma_schdl_set_min_quota(struct mtk_bus_trans *trans, u8 group, u16 quota)
{
	u32 addr;
	u32 value = 0;

	addr = WF_HIF_DMASHDL_TOP_GROUP0_CONTROL_ADDR + (group << 2);

	value = bus_read(trans, addr);

	value &= ~WF_HIF_DMASHDL_TOP_GROUP0_CONTROL_GROUP0_MIN_QUOTA_MASK;
	value |= (quota <<
		  WF_HIF_DMASHDL_TOP_GROUP0_CONTROL_GROUP0_MIN_QUOTA_SHFT) &
		 WF_HIF_DMASHDL_TOP_GROUP0_CONTROL_GROUP0_MIN_QUOTA_MASK;

	bus_write(trans, addr, value);
}

static void
_wfdma_schdl_set_que_mapping(struct mtk_bus_trans *trans, u8 queue, u8 group)
{
	u32 addr, mask, shift;
	u32 value = 0;

	addr = WF_HIF_DMASHDL_TOP_QUEUE_MAPPING0_ADDR +
		 ((queue >> 3) << 2);
	mask = WF_HIF_DMASHDL_TOP_QUEUE_MAPPING0_QUEUE0_MAPPING_MASK <<
		 ((queue % 8) << 2);
	shift = (queue % 8) << 2;

	value = bus_read(trans, addr);

	value &= ~mask;
	value |= (group << shift) & mask;

	bus_write(trans, addr, value);
}

static void
_wfdma_schdl_set_group_priority(struct mtk_bus_trans *trans, u8 group, u8 priority)
{
	u32 addr, mask, shift;
	u32 value = 0;

	addr = WF_HIF_DMASHDL_TOP_HIF_SCHEDULER_SETTING0_ADDR +
		((priority >> 3) << 2);
	mask = WF_HIF_DMASHDL_TOP_HIF_SCHEDULER_SETTING0_PRIORITY0_GROUP_MASK
		 << ((priority % 8) << 2);
	shift = (priority % 8) << 2;

	value = bus_read(trans, addr);

	value &= ~mask;
	value |= (group << shift) & mask;

	bus_write(trans, addr, value);
}

static void
_wfdma_schdl_set_slot_arbiter(struct mtk_bus_trans *trans, bool enable)
{
	u32 value = 0;

	value = bus_read(trans, WF_HIF_DMASHDL_TOP_PAGE_SETTING_ADDR);

	if (enable)
		value |=
		 WF_HIF_DMASHDL_TOP_PAGE_SETTING_GROUP_SEQUENCE_ORDER_TYPE_MASK;
	else
		value &=
		~WF_HIF_DMASHDL_TOP_PAGE_SETTING_GROUP_SEQUENCE_ORDER_TYPE_MASK;

	bus_write(trans, WF_HIF_DMASHDL_TOP_PAGE_SETTING_ADDR, value);
}

static int
bellwether_wfdma_scheduler_init(struct pci_trans *pci_hw)
{
	struct mtk_bus_trans *trans = to_bus_trans(pci_hw);
	u8 idx = 0;

	_wfdma_schdl_set_ple_max_page(trans, dma_schdl_cfg.ple_max_page);

	_wfdma_schdl_set_pse_max_page(trans, dma_schdl_cfg.pse_max_page);

	for (idx = 0; idx < ENUM_DMASHDL_GROUP_NUM; idx++) {
		_wfdma_schdl_set_refill(trans, idx,
				dma_schdl_cfg.refill_enable[idx]);

		_wfdma_schdl_set_max_quota(trans, idx,
				dma_schdl_cfg.max_quota[idx]);

		_wfdma_schdl_set_min_quota(trans, idx,
				dma_schdl_cfg.min_quota[idx]);
	}

	for (idx = 0; idx < ENUM_DMASHDL_GROUP_NUM * 2; idx++)
		_wfdma_schdl_set_que_mapping(trans, idx,
				dma_schdl_cfg.que_grp_mapping[idx]);

	for (idx = 0; idx < ENUM_DMASHDL_GROUP_NUM; idx++)
		_wfdma_schdl_set_group_priority(trans, idx,
				dma_schdl_cfg.pri_grp_mapping[idx]);

	_wfdma_schdl_set_slot_arbiter(trans,
			dma_schdl_cfg.slot_arbiter_enable);

	bus_clear(trans, WF_WFDMA_HOST_DMA0_WPDMA_GLO_CFG_EXT0_ADDR,
		WF_WFDMA_HOST_DMA0_WPDMA_GLO_CFG_EXT0_CSR_TX_DMASHDL_ENABLE_MASK);

	return 0;
}
#endif

#define BELLWETHER_FIRMWARE_ROM_ADDR		0x00800000
#define BELLWETHER_FIRMWARE_ROM_SRAM_ADDR	0xE0048000

static int bellwether_mcu_load_rom(struct mtk_hw_dev *dev)
{
	const struct firmware *fw;
	u32 reg_value;
	int ret = 0;
	u32 offset = 0;

	pr_info("%s start:\n", __func__);

	ret = request_firmware(&fw, BELLWETHER_FIRMWARE_ROM, dev->dev);
	if (ret) {
		dev_err(dev->dev, "Request Rom binary failed.\n");
		return -EINVAL;
		}

	if (!fw || !fw->data) {
		dev_err(dev->dev, "Invalid ROM\n");
		ret = -EINVAL;
		goto out;
		}

	reg_value = bus_read(dev->bus_trans, CONN_INFRA_BUS_CR_ON_CONN_INFRA_PCIE2AP_REMAP_WF__5__4_ADDR);

	bus_write(dev->bus_trans, CONN_INFRA_BUS_CR_ON_CONN_INFRA_PCIE2AP_REMAP_WF__5__4_ADDR,
		field_get(CONN_INFRA_BUS_CR_ON_CONN_INFRA_PCIE2AP_REMAP_WF__5__4_cr_pcie2ap_public_remapping_wf_4_MASK, reg_value) |
		field_prep(CONN_INFRA_BUS_CR_ON_CONN_INFRA_PCIE2AP_REMAP_WF__5__4_cr_pcie2ap_public_remapping_wf_5_MASK, 0x1850));

	while (true) {
		u32 size;

		if (offset >= fw->size)
			break;

		if ((offset + 0x10000) <= fw->size)
			size = 0x10000;
		else
			size = fw->size - offset;

		bus_write(dev->bus_trans, WF_MCU_BUS_CR_AP2WF_REMAP_1_ADDR, BELLWETHER_FIRMWARE_ROM_ADDR + offset);
		bus_write_range(dev->bus_trans, 0x50000, (void *) (fw->data + offset), size);

		offset += 0x10000;
	}

	bus_write(dev->bus_trans, CONN_INFRA_BUS_CR_ON_CONN_INFRA_PCIE2AP_REMAP_WF__5__4_ADDR, reg_value);

out:
	pr_info("%s: ret: %d\n", __func__, ret);
	release_firmware(fw);
	return ret;
}

static int bellwether_mcu_load_rom_sram(struct mtk_hw_dev *dev)
{
	const struct firmware *fw;
	u32 reg_value;
	int ret = 0;
	u32 offset = 0;

	ret = request_firmware(&fw, BELLWETHER_FIRMWARE_ROM_SRAM, dev->dev);
	if (ret) {
		dev_err(dev->dev, "Request Rom binary failed.\n");
		return -EINVAL;
	}

	if (!fw || !fw->data) {
		dev_err(dev->dev, "Invalid ROM\n");
		ret = -EINVAL;
		goto out;
	}

	reg_value = bus_read(dev->bus_trans, CONN_INFRA_BUS_CR_ON_CONN_INFRA_PCIE2AP_REMAP_WF__5__4_ADDR);

	bus_write(dev->bus_trans, CONN_INFRA_BUS_CR_ON_CONN_INFRA_PCIE2AP_REMAP_WF__5__4_ADDR,
		field_get(CONN_INFRA_BUS_CR_ON_CONN_INFRA_PCIE2AP_REMAP_WF__5__4_cr_pcie2ap_public_remapping_wf_4_MASK, reg_value) |
		field_prep(CONN_INFRA_BUS_CR_ON_CONN_INFRA_PCIE2AP_REMAP_WF__5__4_cr_pcie2ap_public_remapping_wf_5_MASK, 0x1850));

	while (true) {
		u32 size;

		if (offset >= fw->size)
			break;

		if ((offset + 0x10000) <= fw->size)
			size = 0x10000;
		else
			size = fw->size - offset;

		pr_info("%s: offset: 0x%x, size: 0x%x\n", __func__, offset, size);

		bus_write(dev->bus_trans, WF_MCU_BUS_CR_AP2WF_REMAP_1_ADDR, BELLWETHER_FIRMWARE_ROM_SRAM_ADDR + offset);
		bus_write_range(dev->bus_trans, 0x50000, (void *) (fw->data + offset), size);

		offset += 0x10000;
	}

	bus_write(dev->bus_trans, CONN_INFRA_BUS_CR_ON_CONN_INFRA_PCIE2AP_REMAP_WF__5__4_ADDR, reg_value);

out:
	pr_info("%s: ret: %d\n", __func__, ret);
	release_firmware(fw);
	return ret;
}

static int bellwether_polling_mcu_idle(struct mtk_hw_dev *dev)
{
#define MCU_IDLE	0x1D1E
#define MCU_IDLE_POLL_ROUND 100
#define MCU_IDLE_POLL_US 1000

	u32 val;
	u8 i = 0;
	bool idle = false;


	do {
		val = bus_read(dev->bus_trans, WF_TOP_CFG_ON_ROMCODE_INDEX_ADDR);
		if (val == MCU_IDLE) {
			idle = true;
			break;
		}

		udelay(MCU_IDLE_POLL_US);
	} while ((i++) < MCU_IDLE_POLL_ROUND);

	pr_info("%s: val: 0x%x, idle: %d\n", __func__, val, idle);

	return !idle;
}

static int bellwether_polling_wf_idle(struct mtk_hw_dev *dev)
{
#define WF_IDLE	0xBE11
#define WF_IDLE_POLL_ROUND 1000
#define WF_IDLE_POLL_US 1000

	u32 val, remap_bk_val;
	u32 i = 0;
	bool idle = false;

	remap_bk_val = bus_read(dev->bus_trans, CONN_INFRA_BUS_CR_ON_CONN_INFRA_PCIE2AP_REMAP_WF__5__4_ADDR);

	bus_write(dev->bus_trans, CONN_INFRA_BUS_CR_ON_CONN_INFRA_PCIE2AP_REMAP_WF__5__4_ADDR,
		field_get(CONN_INFRA_BUS_CR_ON_CONN_INFRA_PCIE2AP_REMAP_WF__5__4_cr_pcie2ap_public_remapping_wf_4_MASK, remap_bk_val) |
		field_prep(CONN_INFRA_BUS_CR_ON_CONN_INFRA_PCIE2AP_REMAP_WF__5__4_cr_pcie2ap_public_remapping_wf_5_MASK, 0x1805));

	/*0x18054A68 -> remap5 -> 0x54A68*/
	do {
		val = bus_read(dev->bus_trans, 0x50000 + 0x4A68);
		if (val == WF_IDLE) {
			idle = true;
			break;
		}

		udelay(WF_IDLE_POLL_US);
	} while ((i++) < WF_IDLE_POLL_ROUND);

	bus_write(dev->bus_trans, CONN_INFRA_BUS_CR_ON_CONN_INFRA_PCIE2AP_REMAP_WF__5__4_ADDR, remap_bk_val);

	pr_info("%s: val: 0x%x, idle: %d\n", __func__, val, idle);

	return !idle;
}

static int bellwether_init_pcie_int(struct mtk_hw_dev *dev)
{
#ifdef CONFIG_HWIFI_CHIP_BELLWETHER_FPGA
	u32 val;

	val = bus_read(dev->bus_trans, CONN_INFRA_BUS_CR_ON_CONN_INFRA_PCIE2AP_REMAP_WF__5__4_ADDR);

	bus_write(dev->bus_trans, CONN_INFRA_BUS_CR_ON_CONN_INFRA_PCIE2AP_REMAP_WF__5__4_ADDR,
		field_get(CONN_INFRA_BUS_CR_ON_CONN_INFRA_PCIE2AP_REMAP_WF__5__4_cr_pcie2ap_public_remapping_wf_4_MASK, val) |
		field_prep(CONN_INFRA_BUS_CR_ON_CONN_INFRA_PCIE2AP_REMAP_WF__5__4_cr_pcie2ap_public_remapping_wf_5_MASK, 0x1804));

	bus_write(dev->bus_trans, 0x50000 + 0x8308, 0x17400);
	bus_write(dev->bus_trans, 0x50000 + 0x8320, 0x0);

	bus_write(dev->bus_trans, CONN_INFRA_BUS_CR_ON_CONN_INFRA_PCIE2AP_REMAP_WF__5__4_ADDR, val);

	bus_write(dev->bus_trans, 0x10D00, 0x10000);

#endif
	return 0;
}

int
bellwether_mcu_start_rom(struct mtk_hw_dev *dev)
{
	int ret;

	/* check rom code dl or not */
	ret = bellwether_polling_mcu_idle(dev);
	if (!ret)
		goto done;

	/* Wf mcu reset */
	bus_rw(dev->bus_trans,
		CONN_INFRA_RGU_ON_WFSYS_CPU_SW_RST_B_ADDR,
		CONN_INFRA_RGU_ON_WFSYS_CPU_SW_RST_B_WFSYS_CPU_SW_RST_B_MASK,
		0);

	ret = bellwether_mcu_load_rom(dev);
	if (ret)
		goto out;

	ret = bellwether_mcu_load_rom_sram(dev);
	if (ret)
		goto out;

	/* FPGA only */
	bellwether_init_pcie_int(dev);

	bus_rw(dev->bus_trans,
		CONN_INFRA_RGU_ON_WFSYS_CPU_SW_RST_B_ADDR,
		CONN_INFRA_RGU_ON_WFSYS_CPU_SW_RST_B_WFSYS_CPU_SW_RST_B_MASK,
		1);

	ret = bellwether_polling_mcu_idle(dev);
	if (ret)
		goto out;

	ret = bellwether_polling_wf_idle(dev);
	if (ret)
		goto out;

done:
	return ret;
out:
	return ret;
}

static int
bellwether_driver_own(struct mtk_hw_dev *dev, struct mtk_bus_trans *trans, bool drv_own)
{
	struct pci_trans *pci_hw = to_pci_trans(trans);
	u32 reg = pci_hw->lp_fw_own_reg_addr;
	u32 target_state = (drv_own ? 0 : 1)
			<< CONN_HOST_CSR_TOP_WF_BAND0_LPCTL_WF_B0_AP_HOST_OWNER_STATE_SYNC_SHFT;
	if (drv_own)
		bus_write(trans, reg,
		CONN_HOST_CSR_TOP_WF_BAND0_LPCTL_WF_B0_AP_HOST_CLR_FW_OWN_HS_PULSE_MASK);
	else
		bus_write(trans, reg,
		CONN_HOST_CSR_TOP_WF_BAND0_LPCTL_WF_B0_AP_HOST_SET_FW_OWN_HS_PULSE_MASK);

	if (!mtk_bus_poll_msec(trans, reg,
		CONN_HOST_CSR_TOP_WF_BAND0_LPCTL_WF_B0_AP_HOST_OWNER_STATE_SYNC_MASK,
		target_state, 500)) {
		dev_err(trans->pdev, "Timeout for %s\n",
			drv_own ? "driver own" : "fw own");

		return -EIO;
	}

	return 0;
}

static int
bellwether_check_fwdl_state(struct mtk_hw_dev *dev, u32 state)
{
	struct mtk_bus_trans *trans = dev->bus_trans;

	dev_info(trans->pdev, "%s(): check target stat=%d, cur state=%d\n",
		__func__, state,
		bus_read(trans, CONN_HOST_CSR_TOP_CONN_ON_MISC_ADDR) &
		CONN_HOST_CSR_TOP_CONN_ON_MISC_DRV_FW_STAT_SYNC_MASK);
	if (!mtk_bus_poll_msec(trans, CONN_HOST_CSR_TOP_CONN_ON_MISC_ADDR,
			CONN_HOST_CSR_TOP_CONN_ON_MISC_DRV_FW_STAT_SYNC_MASK,
			field_prep(CONN_HOST_CSR_TOP_CONN_ON_MISC_DRV_FW_STAT_SYNC_MASK,
			   state), 500)) {
		dev_err(trans->pdev,
			"Timeout for initializing firmware, %x\n",
			bus_read(trans,
			CONN_HOST_CSR_TOP_CONN_ON_MISC_DRV_FW_STAT_SYNC_ADDR));
		return -EIO;
	}
	return 0;
}

static int
bellwether_fwmode_conf(struct mtk_hw_dev *dev)
{
	u32 val;
	struct mtk_bus_trans *trans = dev->bus_trans;

#define SWDEF_MODE_ADDR 0x009143C
#define FW_NORMAL_MODE 0
	/*set as normal mode*/
	val = bus_read(trans, SWDEF_MODE_ADDR);
	bus_write(trans, SWDEF_MODE_ADDR, FW_NORMAL_MODE);
	return 0;
}

#define L1_REMAP_ADDR		0xFE418
#define L1_REMAP_MASK		0x0000FFFF
#define L1_REMAP_SHFT		0
#define L1_REMAP_BASE_ADDR	0x40000
#define L1_REMAP_BASE_SHFT	16
#define L1_REMAP_BASE_MASK	0xFFFF
#define GET_L1_REMAP_BASE(addr) ((addr) >> L1_REMAP_BASE_SHFT)
#define GET_L1_REMAP_OFFSET(addr) ((addr) & L1_REMAP_BASE_MASK)

/*
 * static u32
 * bellwether_l1_remap_read(struct pci_trans *trans, u32 offset)
 * {
 *	u32 bk_val, tmp_val;
 *
 *	bk_val = bus_read(to_bus_trans(trans), L1_REMAP_ADDR);
 *	tmp_val = bk_val & ~L1_REMAP_MASK;
 *	tmp_val |= GET_L1_REMAP_BASE(offset) << L1_REMAP_SHFT;
 *	bus_write(to_bus_trans(trans), L1_REMAP_ADDR, tmp_val);
 *	tmp_val = GET_L1_REMAP_OFFSET(offset);
 *	tmp_val = bus_read(to_bus_trans(trans), L1_REMAP_BASE_ADDR + tmp_val);
 *	bus_write(to_bus_trans(trans), L1_REMAP_ADDR, bk_val);
 *
 *	return tmp_val;
 * }
 */

static void
bellwether_l1_remap_write(struct pci_trans *trans, u32 offset, u32 value)
{
	u32 bk_val, tmp_val;

	bk_val = bus_read(to_bus_trans(trans), L1_REMAP_ADDR);
	tmp_val = bk_val & ~L1_REMAP_MASK;
	tmp_val |= GET_L1_REMAP_BASE(offset) << L1_REMAP_SHFT;
	bus_write(to_bus_trans(trans), L1_REMAP_ADDR, tmp_val);
	tmp_val = GET_L1_REMAP_OFFSET(offset);
	bus_write(to_bus_trans(trans), L1_REMAP_BASE_ADDR + tmp_val, value);
	bus_write(to_bus_trans(trans), L1_REMAP_ADDR, bk_val);
}

static void
bellwether_hw_caps_init(struct mtk_hw_dev *dev)
{
	dev->limit |= BIT(LIMIT_SET_BMC_WCID);
	dev->band_idx_bmp_sup = BIT(BAND0) | BIT(BAND2);
	dev->band_num_sup = hweight8(dev->band_idx_bmp_sup);
}

static int
bellwether_hw_init(struct mtk_hw_dev *dev)
{
	int ret = 0;

	bmac_ops_init(dev);
	bellwether_hw_caps_init(dev);
	ret = bellwether_mcu_start_rom(dev);
	if (ret)
		goto out;

	ret = bellwether_fwmode_conf(dev);
	if (ret)
		goto out;
out:
	return ret;
}

static int
bellwether_hw_reset(struct mtk_hw_dev *dev)
{
	struct pci_trans *trans = to_pci_trans(dev->bus_trans);

#define WF_SUBSYS_RST 0x70002600

	bellwether_l1_remap_write(trans, WF_SUBSYS_RST, 0x10);
	udelay(1000);
	bellwether_l1_remap_write(trans, WF_SUBSYS_RST, 0x0);

	return 0;
}

static struct pci_device_id bellwether_pci_device_table[] = {
	{ PCI_DEVICE(0x14c3, BELLWETHER_CHIP_ID) },
	{ },
};

static struct mtk_chip_ctrl_ops bellwether_chip_ops = {
	.driver_own = bellwether_driver_own,
	.check_fwdl_state = bellwether_check_fwdl_state,
	.set_filter = NULL,
	.mac_mib_update = NULL,
	.hw_init = bellwether_hw_init,
	.hw_reset = bellwether_hw_reset,
};

static struct mtk_hwres_radio_info bellwether_band0[] = {
	{
		.type = HWIFI_PHY_2G,
		.cap = HWIFI_PHY_LEGACY |
			HWIFI_PHY_N |
			HWIFI_PHY_HE |
			HWIFI_PHY_BE
	},
	{
		.type = HWIFI_PHY_5G,
		.cap = HWIFI_PHY_LEGACY |
			HWIFI_PHY_N |
			HWIFI_PHY_VHT |
			HWIFI_PHY_HE |
			HWIFI_PHY_BE
	}
};

static struct mtk_hwres_radio_info bellwether_band2[] = {
	{
		.type = HWIFI_PHY_2G,
		.cap = HWIFI_PHY_LEGACY |
			HWIFI_PHY_N |
			HWIFI_PHY_HE |
			HWIFI_PHY_BE
	},
	{
		.type = HWIFI_PHY_5G,
		.cap = HWIFI_PHY_LEGACY |
			HWIFI_PHY_N |
			HWIFI_PHY_VHT |
			HWIFI_PHY_HE |
			HWIFI_PHY_BE
	}
};

#ifdef CONFIG_HWIFI_MOBILE_SUPPORT
static struct mtk_mcu_dest_2_path bellwether_mobile_dest_2_path[MCU_DEST_MAX_NUM] = {
	{MCU_DEST_WM,		MCU_PATH_WM},
	{MCU_DEST_RVD,		MCU_PATH_WA},
	{MCU_DEST_WA,		MCU_PATH_WA},
	{MCU_DEST_WA_WM,	MCU_PATH_WA},
	{MCU_DEST_WO,		MCU_PATH_WO},
};

static struct mtk_hwres_radio_cap bellwether_mobile_hw_radio_cap[] = {
	{
		/*band_idx 0*/
		.radio_info_size = ARRAY_SIZE(bellwether_band0),
		.info = bellwether_band0,
		.band_idx = 0,
		.wmm_set = {
			.start = 0,
			.end = 0
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
		.max_tx_tk_nums = 16384,
	},
	{
		/*band_idx 1*/
		.radio_info_size = ARRAY_SIZE(bellwether_band1),
		.info = bellwether_band1,
		.band_idx = 1,
		.wmm_set = {
			.start = 0,
			.end = 0
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
		.max_tx_tk_nums = 16384,
	},
	{
		/*band_idx 2*/
		.radio_info_size = ARRAY_SIZE(bellwether_band2),
		.info = bellwether_band2,
		.band_idx = 2,
		.wmm_set = {
			.start = 0,
			.end = 0
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
		.max_tx_tk_nums = 16384,
	},
};

static struct mtk_hwres_cap bellwether_mobile_hw_res = {
	.uwtbl = {
		.start = 1,
		.end = 200,
	},
	.group = {
		.start = 201,
		.end = 255,
	},
	.tx_token = {
		.start = 0,
		.end = 16383,
	},
	.rx_token = {
		.start = 0,
		.end = 7167,
	},
	.bss = {
		.start = 0,
		.end = 63,
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
	.radio_num = ARRAY_SIZE(bellwether_mobile_hw_radio_cap),
	.radio_cap = bellwether_mobile_hw_radio_cap,
};

static struct mtk_chip_hw_cap bellwether_mobile_chip_cap = {
	.dev_size = sizeof(struct mtk_hw_dev),
	.fw_flags = BIT(FW_FLAG_WM),
	.mac_type = MAC_TYPE_BMAC,
	.mtxd_sz = 64,
	.mac_rxd_grp_0_sz = 8 * 4,
	.hwres = &bellwether_mobile_hw_res,
	.mcu_infos = {
		[MCU_WM] = {
			.fw[MCU_FW_MODE_NORMALMODE] = BELLWETHER_FIRMWARE_WM,
			.rom_patch = BELLWETHER_ROM_PATCH,
			.opt = BIT(WM_OPT_BYPASS_WA)
		},
	}
};

static struct pci_chip_ops bellwether_mobile_pci_ops = {
	.dma_enable = bellwether_pdma_enable,
	.dma_disable = bellwether_pdma_disable,
	.dma_init = bellwether_pdma_init,
	.dma_match = NULL,
	.dma_schdl_init = bellwether_wfdma_scheduler_init,
};

static struct mtk_chip_drv bellwether_mobile_drv = {
	.bus_cfg = {
		.bus_type = MTK_BUS_PCI,
		.id_table = bellwether_pci_device_table,
		.name = KBUILD_MODNAME,
		.profile = &bellwether_pci_profile,
		.ms_type = CHIP_TYPE_MASTER,
		.bus_ops = &bellwether_mobile_pci_ops,
		.bus_drv_sz = sizeof(struct pci_driver),
	},
	.ctl_ops = &bellwether_chip_ops,
	.hw_caps = &bellwether_mobile_chip_cap,
	.device_id = BELLWETHER_CHIP_ID,
	.dest_2_path = bellwether_mobile_dest_2_path,
};
#endif /*CONFIG_HWIFI_MOBILE_SUPPORT*/

#ifdef CONFIG_HWIFI_REBB_SUPPORT
static struct mtk_mcu_dest_2_path bellwether_rebb_dest_2_path[MCU_DEST_MAX_NUM] = {
	{MCU_DEST_WM,		MCU_PATH_WA},
	{MCU_DEST_RVD,		MCU_PATH_WA},
	{MCU_DEST_WA,		MCU_PATH_WA},
	{MCU_DEST_WA_WM,	MCU_PATH_WA},
	{MCU_DEST_WO,		MCU_PATH_WO},
};

static struct mtk_hwres_radio_cap bellwether_rebb_hw_radio_cap[] = {
	{
		/*band_idx 0*/
		.radio_info_size = ARRAY_SIZE(bellwether_band0),
		.info = bellwether_band0,
		.band_idx = 0,
		.wmm_set = {
			.start = 0,
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
		.max_tx_tk_nums = 16384,
	},
	{
		/*band_idx 2*/
		.radio_info_size = ARRAY_SIZE(bellwether_band2),
		.info = bellwether_band2,
		.band_idx = 2,
		.wmm_set = {
			.start = 0,
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
		.max_tx_tk_nums = 16384,
	},
};

static struct mtk_hwres_cap bellwether_rebb_hw_res = {
	.uwtbl = {
		.start = 1,
		.end = 200,
	},
	.group = {
		.start = 201,
		.end = 255,
	},
	.tx_token = {
		.start = 0,
		.end = 16383,
	},
	.rx_token = {
		.start = 0,
		.end = 7167,
	},
	.bss = {
		.start = 0,
		.end = 63,
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
	.radio_num = ARRAY_SIZE(bellwether_rebb_hw_radio_cap),
	.radio_cap = bellwether_rebb_hw_radio_cap,
	.tx_free_done_ver = FREE_NOTIFY_VERSION_4,
};

static struct mtk_chip_hw_cap bellwether_rebb_chip_cap = {
	.dev_size = sizeof(struct mtk_hw_dev),
	.fw_flags = BIT(FW_FLAG_WM) | BIT(FW_FLAG_WA),
	.mac_type = MAC_TYPE_BMAC,
	.mtxd_sz = 148,
	.mrxd_sz = 0x60,
	.mac_rxd_grp_0_sz = 8 * 4,
	.hwres = &bellwether_rebb_hw_res,
	.mcu_infos = {
		[MCU_WM] = {
			.fw = {
				[MCU_FW_MODE_NORMALMODE] = BELLWETHER_FIRMWARE_WM,
				[MCU_FW_MODE_TESTMODE] = BELLWETHER_FIRMWARE_WM_TESTMODE,
			},
			.fw_mode = MCU_FW_MODE_NORMALMODE,
			.rom_patch = BELLWETHER_ROM_PATCH,
			.opt = 0
		},
		[MCU_WA] = {
			.fw = {
				[MCU_FW_MODE_NORMALMODE] = BELLWETHER_FIRMWARE_WA,
				[MCU_FW_MODE_TESTMODE] = NULL,
			},
			.fw_mode = MCU_FW_MODE_NORMALMODE,
			.rom_patch = NULL,
			.opt = 0
		},
		[MCU_DSP] = {
			.fw = {
				[MCU_FW_MODE_NORMALMODE] = NULL,
				[MCU_FW_MODE_TESTMODE] = NULL,
			},
			.fw_mode = MCU_FW_MODE_MAX,
			.rom_patch = NULL,
			.opt = 0
		},
	},
	.cap_info = {
		.sw_max_amsdu_num = 13,
		.sw_max_amsdu_len = 1536,
	},
#ifdef CONFIG_HWIFI_SW_AMSDU
	.mac_cap = BIT(CAP_PAO) | BIT(CAP_OFFLOAD_TXD),
	.hif_txd_ver = HIF_TXD_V2_0,
#else
	.mac_cap = BIT(CAP_OFFLOAD_TXD),
	.hif_txd_ver = HIF_TXD_V1_0,
#endif
};

static struct pci_chip_ops bellwether_rebb_pci_ops = {
	.dma_enable = bellwether_pdma_enable,
	.dma_disable = bellwether_pdma_disable,
	.dma_init = bellwether_pdma_init,
	.dma_match = NULL,
	.dma_schdl_init = NULL,
};

static struct mtk_chip_drv bellwether_rebb_drv = {
	.bus_cfg = {
		.bus_type = MTK_BUS_PCI,
		.id_table = bellwether_pci_device_table,
		.name = KBUILD_MODNAME,
		.profile = &bellwether_pci_wa_profile,
		.ms_type = CHIP_TYPE_MASTER,
		.bus_ops = &bellwether_rebb_pci_ops,
		.bus_drv_sz = sizeof(struct pci_driver),
	},
	.ctl_ops = &bellwether_chip_ops,
	.hw_caps = &bellwether_rebb_chip_cap,
	.device_id = BELLWETHER_CHIP_ID,
	.dest_2_path = bellwether_rebb_dest_2_path,
};
#endif /*CONFIG_HWIFI_REBB_SUPPORT*/

static int __init mtk_chip_bellwether_init(void)
{
	static struct mtk_chip_drv *drv;

#ifdef CONFIG_HWIFI_MOBILE_SUPPORT
	drv = &bellwether_mobile_drv;
#endif
#ifdef CONFIG_HWIFI_REBB_SUPPORT
	drv = &bellwether_rebb_drv;
#endif

#ifdef CONFIG_HWIFI_HIF_WED
	bellwether_rebb_drv.bus_cfg.bus_type = MTK_BUS_WED;
	bellwether_rebb_drv.bus_cfg.bus_sub_type = MTK_BUS_PCI;
	bellwether_rebb_drv.bus_cfg.bus_drv_sz = sizeof(struct pci_driver);
	bellwether_rebb_hw_res.tx_token.start = 8192;
	bellwether_rebb_hw_res.tx_token.end = 9215;
	bellwether_rebb_hw_res.ext_tx_tk_nums_master = 8192; /* TKID num */
	bellwether_rebb_hw_res.ext_rx_tk_nums_master = 8192; /* RXBM buffer num */
	bellwether_rebb_hw_res.ext_tx_buf_nums_master = 8192; /* TXBM buffer num */
	bellwether_rebb_hw_radio_cap[0].max_tx_tk_nums = 1024;
	bellwether_rebb_hw_radio_cap[1].max_tx_tk_nums = 1024;

	bellwether_rebb_hw_res.rx_token.end += bellwether_rebb_hw_res.ext_rx_tk_nums_master;
#endif

	drv->interface_type = mac_type;

	return mtk_chip_register(drv);
}

static void __exit mtk_chip_bellwether_exit(void)
{
	static struct mtk_chip_drv *drv;

#ifdef CONFIG_HWIFI_MOBILE_SUPPORT
	drv = &bellwether_mobile_drv;
#endif
#ifdef CONFIG_HWIFI_REBB_SUPPORT
	drv = &bellwether_rebb_drv;
#endif

	mtk_chip_unregister(drv);
}

module_init(mtk_chip_bellwether_init);
module_exit(mtk_chip_bellwether_exit);

MODULE_LICENSE("Dual BSD/GPL");
MODULE_DESCRIPTION("Mediatek Hardware Wi-Fi Module");
