/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef MT6990_HNAT_LAN_ACCEL_H
#define MT6990_HNAT_LAN_ACCEL_H
int hnat_lan_accel_init(void);
void hnat_lan_accel_exit(void);
bool hnat_cpu_wifi_pingpong(struct sk_buff *skb);
#endif
