/*
 * Driver interaction with Linux nl80211/cfg80211
 * Copyright (c) 2002-2010, Jouni Malinen <j@w1.fi>
 * Copyright (c) 2003-2004, Instant802 Networks, Inc.
 * Copyright (c) 2005-2006, Devicescape Software, Inc.
 * Copyright (c) 2007, Johannes Berg <johannes@sipsolutions.net>
 * Copyright (c) 2009-2010, Atheros Communications
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * Alternatively, this software may be distributed under the terms of BSD
 * license.
 *
 * See README and COPYING for more details.
 */

#ifndef _MTK_DRIVER_NL80211_H_
#define _MTK_DRIVER_NL80211_H_

#include <linux/wireless.h>

#ifndef BITS
/* Eddie */
/* bits range: for example BITS(16,23) = 0xFF0000
 *   ==>  (BIT(m)-1)   = 0x0000FFFF     ~(BIT(m)-1)   => 0xFFFF0000
 *   ==>  (BIT(n+1)-1) = 0x00FFFFFF
 */
#define BITS(m,n)                       (~(BIT(m)-1) & ((BIT(n) - 1) | BIT(n)))
#endif /* BIT */
#ifndef OUI_MTK
#define OUI_MTK 0x000CE7
#endif
extern void nl80211_vendor_event_mtk(struct wpa_driver_nl80211_data *, struct i802_bss *, u32, u8 *, size_t);
enum mtk_generic_response_element {
    MTK_GRID_MLO_EXTERNAL_AUTH = 1,                         /* 1 */
};

struct mtk_externa_auth_info {
    uint8_t ssid[SSID_MAX_LEN + 1];
    uint8_t ssid_len;
    uint8_t bssid[ETH_ALEN];
    uint32_t key_mgmt_suite;
    uint32_t action;
    uint8_t da[ETH_ALEN];
    uint8_t ext_ie[0];
} STRUCT_PACKED;
#endif

