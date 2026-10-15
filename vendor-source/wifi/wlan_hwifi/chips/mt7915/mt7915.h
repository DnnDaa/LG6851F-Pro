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

#ifndef __MTK_CHIP_MT7915_H
#define __MTK_CHIP_MT7915_H

#include <chips.h>
#include <bus.h>
#include <core.h>
#include <bus/mtk_pci_dma.h>
#include <bus/mtk_wed.h>
#include "mt7915_wfdma_wa.h"

#define MT7915_FIRMWARE_WA		"mediatek/mt7915_wa.bin"
#define MT7915_FIRMWARE_WM		"mediatek/mt7915_wm.bin"
#define MT7915_ROM_PATCH		"mediatek/mt7915_rom_patch.bin"

/* RMAC: band 0(0x21400), band 1(0xa1400) */
#define MT_WF_RMAC_BASE(_band)		((_band) ? 0xa1400 : 0x21400)
#define MT_WF_RMAC(_band, ofs)		(MT_WF_RMAC_BASE(_band) + (ofs))

#define MT_WF_RFCR(_band)		MT_WF_RMAC(_band, 0x000)
#define MT_WF_RFCR_DROP_STBC_MULTI	BIT(0)
#define MT_WF_RFCR_DROP_FCSFAIL		BIT(1)
#define MT_WF_RFCR_DROP_VERSION		BIT(3)
#define MT_WF_RFCR_DROP_PROBEREQ	BIT(4)
#define MT_WF_RFCR_DROP_MCAST		BIT(5)
#define MT_WF_RFCR_DROP_BCAST		BIT(6)
#define MT_WF_RFCR_DROP_MCAST_FILTERED	BIT(7)
#define MT_WF_RFCR_DROP_A3_MAC		BIT(8)
#define MT_WF_RFCR_DROP_A3_BSSID	BIT(9)
#define MT_WF_RFCR_DROP_A2_BSSID	BIT(10)
#define MT_WF_RFCR_DROP_OTHER_BEACON	BIT(11)
#define MT_WF_RFCR_DROP_FRAME_REPORT	BIT(12)
#define MT_WF_RFCR_DROP_CTL_RSV		BIT(13)
#define MT_WF_RFCR_DROP_CTS		BIT(14)
#define MT_WF_RFCR_DROP_RTS		BIT(15)
#define MT_WF_RFCR_DROP_DUPLICATE	BIT(16)
#define MT_WF_RFCR_DROP_OTHER_BSS	BIT(17)
#define MT_WF_RFCR_DROP_OTHER_UC	BIT(18)
#define MT_WF_RFCR_DROP_OTHER_TIM	BIT(19)
#define MT_WF_RFCR_DROP_NDPA		BIT(20)
#define MT_WF_RFCR_DROP_UNWANTED_CTL	BIT(21)

#define MT_WF_RFCR1(_band)		MT_WF_RMAC(_band, 0x004)
#define MT_WF_RFCR1_DROP_ACK		BIT(4)
#define MT_WF_RFCR1_DROP_BF_POLL	BIT(5)
#define MT_WF_RFCR1_DROP_BA		BIT(6)
#define MT_WF_RFCR1_DROP_CFEND		BIT(7)
#define MT_WF_RFCR1_DROP_CFACK		BIT(8)

/* MIB: band 0(0x24800), band 1(0xa4800) */
#define MT_WF_MIB_BASE(_band)		((_band) ? 0xa4800 : 0x24800)
#define MT_WF_MIB(_band, ofs)		(MT_WF_MIB_BASE(_band) + (ofs))

#define MT_MIB_SDR3(_band)		MT_WF_MIB(_band, 0x014)
#define MT_MIB_SDR3_FCS_ERR_MASK	GENMASK(15, 0)

#define MT_MIB_SDR9(_band)		MT_WF_MIB(_band, 0x02c)
#define MT_MIB_SDR9_BUSY_MASK		GENMASK(23, 0)

#define MT_MIB_SDR16(_band)		MT_WF_MIB(_band, 0x048)
#define MT_MIB_SDR16_BUSY_MASK		GENMASK(23, 0)

#define MT_MIB_SDR34(_band)		MT_WF_MIB(_band, 0x090)
#define MT_MIB_MU_BF_TX_CNT		GENMASK(15, 0)

#define MT_MIB_SDR36(_band)		MT_WF_MIB(_band, 0x098)
#define MT_MIB_SDR36_TXTIME_MASK	GENMASK(23, 0)
#define MT_MIB_SDR37(_band)		MT_WF_MIB(_band, 0x09c)
#define MT_MIB_SDR37_RXTIME_MASK	GENMASK(23, 0)

#define MT_MIB_DR8(_band)		MT_WF_MIB(_band, 0x0c0)
#define MT_MIB_DR9(_band)		MT_WF_MIB(_band, 0x0c4)
#define MT_MIB_DR11(_band)		MT_WF_MIB(_band, 0x0cc)

#define MT_MIB_MB_SDR0(_band, n)	MT_WF_MIB(_band, 0x100 + ((n) << 4))
#define MT_MIB_RTS_RETRIES_COUNT_MASK	GENMASK(31, 16)
#define MT_MIB_RTS_COUNT_MASK		GENMASK(15, 0)

#define MT_MIB_MB_SDR1(_band, n)	MT_WF_MIB(_band, 0x104 + ((n) << 4))
#define MT_MIB_BA_MISS_COUNT_MASK	GENMASK(15, 0)
#define MT_MIB_ACK_FAIL_COUNT_MASK	GENMASK(31, 16)

#define MT_MIB_MB_SDR2(_band, n)	MT_WF_MIB(_band, 0x108 + ((n) << 4))
#define MT_MIB_FRAME_RETRIES_COUNT_MASK	GENMASK(15, 0)

#define MT_TX_AGG_CNT(_band, n)		MT_WF_MIB(_band, 0x0a8 + ((n) << 2))
#define MT_TX_AGG_CNT2(_band, n)	MT_WF_MIB(_band, 0x164 + ((n) << 2))
#define MT_MIB_ARNG(_band, n)		MT_WF_MIB(_band, 0x4b8 + ((n) << 2))
#define MT_MIB_ARNCR_RANGE(val, n)	(((val) >> ((n) << 3)) & GENMASK(7, 0))

static inline u32
mt7915_reg_map_l1(struct mtk_bus_trans *trans, u32 addr)
{
#define MT_HIF_REMAP_L1_OFFSET  GENMASK(15, 0)
#define MT_HIF_REMAP_L1_BASE	GENMASK(31, 16)
#define MT_HIF_REMAP_BASE_L1	0xe0000

	u32 offset = field_get(MT_HIF_REMAP_L1_OFFSET, addr);
	u32 base = field_get(MT_HIF_REMAP_L1_BASE, addr);

	bus_set_field(trans, CONN_INFRA_CFG_CONN_INFRA_PCIE2AP_REMAP_7_r_pcie2ap_public_remapping_e_ADDR,
		CONN_INFRA_CFG_CONN_INFRA_PCIE2AP_REMAP_7_r_pcie2ap_public_remapping_e_MASK, base);
	bus_read(trans, CONN_INFRA_CFG_CONN_INFRA_PCIE2AP_REMAP_7_r_pcie2ap_public_remapping_e_ADDR);

	return MT_HIF_REMAP_BASE_L1 + offset;
}

static inline u32
mt7915_l1_rr(struct mtk_bus_trans *trans, u32 addr)
{
	return bus_read(trans, mt7915_reg_map_l1(trans, addr));
}

static inline void
mt7915_l1_wr(struct mtk_bus_trans *trans, u32 addr, u32 val)
{
	bus_write(trans, mt7915_reg_map_l1(trans, addr), val);
}

static inline u32
mt7915_reg_map_l2(struct mtk_bus_trans *trans, u32 addr)
{
#define MT_HIF_REMAP_L2_OFFSET		GENMASK(11, 0)
#define MT_HIF_REMAP_L2_BASE		GENMASK(31, 12)
#define MT_HIF_REMAP_BASE_L2		0x00000

	u32 offset = field_get(MT_HIF_REMAP_L2_OFFSET, addr);
	u32 base = field_get(MT_HIF_REMAP_L2_BASE, addr);

	bus_set_field(trans,
		CONN_INFRA_CFG_CONN_INFRA_LEGACY_REMAP_CTRL_0_r_connac_legacy_ap2wf_addr_map0_ADDR,
		CONN_INFRA_CFG_CONN_INFRA_LEGACY_REMAP_CTRL_0_r_connac_legacy_ap2wf_addr_map0_MASK,
		base);
	/* use read to push write */
	bus_read(trans, CONN_INFRA_CFG_CONN_INFRA_LEGACY_REMAP_CTRL_0_r_connac_legacy_ap2wf_addr_map0_ADDR);

	return MT_HIF_REMAP_BASE_L2 + offset;
}

static inline u32
mt7915_l2_rr(struct mtk_bus_trans *trans, u32 addr)
{
	return bus_read(trans, mt7915_reg_map_l2(trans, addr));
}

static inline void
mt7915_l2_wr(struct mtk_bus_trans *trans, u32 addr, u32 val)
{
	bus_write(trans, mt7915_reg_map_l2(trans, addr), val);
}

/*2 port PCI usage*/
#define DEFAULT_RID 0
#define MASTER_RID 0x7915

static inline u32
get_rid_value(struct pci_trans *trans)
{
	u32 rid;

	rid = bus_read(to_bus_trans(trans), WF_WFDMA_EXT_WRAP_CSR_PCIE_RECOGNITION_ID_PCIE_RECOG_ID_ADDR);

	dev_info(to_device(trans), "%s(): rid =%x\n", __func__, rid);
	return ((rid & WF_WFDMA_EXT_WRAP_CSR_PCIE_RECOGNITION_ID_PCIE_RECOG_ID_MASK) >>
		WF_WFDMA_EXT_WRAP_CSR_PCIE_RECOGNITION_ID_PCIE_RECOG_ID_SHFT);
}

static inline void
set_rid_value(struct pci_trans *trans, u32 rid)
{
	u32 value = WF_WFDMA_EXT_WRAP_CSR_PCIE_RECOGNITION_ID_PCIE_RECOG_SEM_MASK | rid;

	bus_write(to_bus_trans(trans), WF_WFDMA_EXT_WRAP_CSR_PCIE_RECOGNITION_ID_ADDR, value);
}

extern void fmac_ops_init(struct mtk_hw_dev *dev);

#endif
