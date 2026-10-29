/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef LG6851F_WIFI_ACCEL_POLICY_H
#define LG6851F_WIFI_ACCEL_POLICY_H
/* The provider ABI is six bits, not the AP's SSID number or ifindex. */
#define CPU_WIFI_INDEX_BITS 6
#define CPU_WIFI_INDEX_MASK ((1U << CPU_WIFI_INDEX_BITS) - 1)
#define CPU_WIFI_GENERATION_MASK 31U
#define CPU_WIFI_VID_BASE 0x800
#define CPU_WIFI_GENERATION_GUARD (30 * HZ)

static inline bool wifi_device(const struct net_device *dev)
{
 const struct wireless_dev *wdev = dev ? dev->ieee80211_ptr : NULL;
 return wdev && wdev->wiphy && wdev->iftype == NL80211_IFTYPE_AP &&
        dev->type == ARPHRD_ETHER && net_eq(dev_net(dev), &init_net);
}

static inline unsigned int cpu_wifi_tag(unsigned int index, unsigned int generation)
{
 return CPU_WIFI_VID_BASE | (generation << CPU_WIFI_INDEX_BITS) | index;
}
#endif
