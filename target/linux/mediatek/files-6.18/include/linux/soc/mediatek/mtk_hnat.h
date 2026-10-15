/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef _LINUX_SOC_MEDIATEK_MTK_HNAT_H
#define _LINUX_SOC_MEDIATEK_MTK_HNAT_H
#include <linux/types.h>
#include <linux/kconfig.h>
struct sk_buff;
struct mtk_hnat_wifi_ops {
 /* true means the provider consumed the learning/keepalive packet. */
 bool (*tx)(struct sk_buff *skb, u8 wdma, u8 bss, u16 wcid, u8 ring);
 void (*flush)(const u8 *mac);
};
#if IS_ENABLED(CONFIG_HW_NAT)
int mtk_hnat_wifi_register(const struct mtk_hnat_wifi_ops *ops);
void mtk_hnat_wifi_unregister(const struct mtk_hnat_wifi_ops *ops);
bool mtk_hnat_wifi_tx(struct sk_buff *skb, u8 wdma, u8 bss, u16 wcid, u8 ring);
void mtk_hnat_wifi_flush(const u8 *mac);
#else
static inline bool mtk_hnat_wifi_tx(struct sk_buff *skb, u8 wdma, u8 bss,
                                  u16 wcid, u8 ring) { return false; }
static inline void mtk_hnat_wifi_flush(const u8 *mac) {}
#endif
#endif
