/*******************************************************************************
 *
 * This file is provided under a dual license.  When you use or
 * distribute this software, you may choose to be licensed under
 * version 2 of the GNU General Public License ("GPLv2 License")
 * or BSD License.
 *
 * GPLv2 License
 *
 * Copyright(C) 2016 MediaTek Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of version 2 of the GNU General Public License as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See http://www.gnu.org/licenses/gpl-2.0.html for more details.
 *
 * BSD LICENSE
 *
 * Copyright(C) 2016 MediaTek Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 *  * Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *  * Neither the name of the copyright holder nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 ******************************************************************************/

#ifndef WPA_ML_COMMON_H
#define WPA_ML_COMMON_H

#include "common/wpa_common.h"

struct hostapd_data;
struct wpa_state_machine;
struct wpa_authenticator;
struct wpa_sm;
struct sae_data;

#ifndef OUI_MTK
#define OUI_MTK 0x000ce7
#endif

#define MTK_MLD_IE_VENDOR_TYPE 0x000ce701


#define MTK_VENDOR_ATTR_P2P_ML_CHAN_LIST  0

#define ML_CTRL_TYPE_MASK				BITS(0, 2)
#define ML_CTRL_TYPE_BASIC				0
#define ML_ELEMENT_TYPE_PROBE_REQ			1

#define ML_CTRL_PRE_BMP_MASK				BITS(4, 15)
#define ML_CTRL_LINK_ID_INFO_PRESENT			BIT(4)
#define ML_CTRL_BSS_PARA_CHANGE_COUNT_PRESENT		BIT(5)
#define ML_CTRL_MEDIUM_SYN_DELAY_INFO_PRESENT		BIT(6)
#define ML_CTRL_EML_CAPA_PRESENT			BIT(7)
#define ML_CTRL_MLD_CAPA_PRESENT			BIT(8)
#define ML_CTRL_MLD_ID_PRESENT				BIT(9)
#define ML_CTRL_EXT_MLD_CAP_PRESENT                     BIT(10)

#define ML_SUB_ID_PER_STA_PROFILE			0
#define ML_STA_CTRL_LINK_ID_MASK			BITS(0, 3)
#define ML_STA_CTRL_LINK_ID_SHIFT			0
#define ML_STA_CTRL_COMPLETE_PROFILE			BIT(4)
#define ML_STA_CTRL_MAC_ADDR_PRESENT			BIT(5)
#define ML_STA_CTRL_BCN_INTV_PRESENT			BIT(6)
#define ML_STA_CTRL_TSF_OFFSET_PRESENT			BIT(7)
#define ML_STA_CTRL_DTIM_INFO_PRESENT			BIT(8)
#define ML_STA_CTRL_NSTR_LINK_PAIR_PRESENT		BIT(9)
#define ML_STA_CTRL_NSTR_BMP_SIZE			BIT(10)
#define ML_STA_CTRL_NSTR_BMP_SIZE_SHIFT		10
#define ML_STA_CTRL_BSS_CHG_CNT_PRESENT		BIT(11)

#define ML_SET_CTRL_TYPE(_u2ctrl, _ctrl_type) \
{\
	(_u2ctrl) &= ~(ML_CTRL_TYPE_MASK); \
	(_u2ctrl) |= ((_ctrl_type) & (ML_CTRL_TYPE_MASK)); \
}

#define ML_SET_CTRL_PRESENCE(_u2ctrl, _ctrl_type) \
{\
	(_u2ctrl) &= ~(ML_CTRL_PRE_BMP_MASK); \
	(_u2ctrl) |= ((_ctrl_type) & (ML_CTRL_PRE_BMP_MASK)); \
}


#define ML_IS_CTRL_TYPE(__ie, __TYPE) \
	((__ie) && (__ie)[0] == WLAN_EID_EXTENSION && (__ie)[1] >= 3 && \
	 (__ie)[2] == WLAN_EID_EXT_MULTI_LINK && \
	 ((__ie)[3] & ML_CTRL_TYPE_MASK) == __TYPE)


#define WPA_MLO_GTK_KDE_PREFIX_LEN (1 + 6)
struct wpa_mlo_gtk_kde {
	u8 info; /* KeyId 2 | Tx 1 | Reserved 1 | LinkId 4 */
	u8 pn[6];
	u8 gtk[WPA_GTK_MAX_LEN];
} STRUCT_PACKED;

#define WPA_MLO_IGTK_KDE_PREFIX_LEN (2 + 6 + 1)
struct wpa_mlo_igtk_kde {
	u8 keyid[2];
	u8 pn[6];
	u8 info; /* Reserved 4 | LinkId 4 */
	u8 igtk[WPA_IGTK_MAX_LEN];
} STRUCT_PACKED;

#define WPA_MLO_BIGTK_KDE_PREFIX_LEN (2 + 6 + 1)
struct wpa_mlo_bigtk_kde {
	u8 keyid[2];
	u8 pn[6];
	u8 info; /* Reserved 4 | LinkId 4 */
	u8 bigtk[WPA_BIGTK_MAX_LEN];
} STRUCT_PACKED;

struct wpa_mlo_link_kde {
	u8 info; /* LinkId 4 | RSNEInfo 1 | RSNXEInfo 1 | Reserved 2 */
	u8 addr[ETH_ALEN];
	u8 var[]; /* RSNE | RSNXE */
} STRUCT_PACKED;


struct per_sta_profile {
	u8 link_id;
	u8 addr[ETH_ALEN];
	u16 dtim;
	u16 beacon_interval;
	u64 tsf_offset;
	u16 nstr_bmap;
	u8 bss_para_change_count;
	unsigned int complete_profile:1;
	unsigned int mac_addr_present:1;
	unsigned int bcn_intvl_present:1;
	unsigned int tsf_offset_present:1;
	unsigned int dtim_present:1;
	unsigned int nstr_present:1;
	unsigned int bss_para_change_count_present:1;
};

struct wpa_ml_ie_parse {
	u8 type;
	u8 ml_addr[ETH_ALEN];
	u8 common_info_len;
	u8 link_id;
	u8 bss_para_change_count;
	u16 medium_sync_delay;
	u16 eml_cap;
	u16 mld_cap;
	u8 mld_id;
	u8 prof_num;
	unsigned int valid:1;
	unsigned int link_id_present:1;
	unsigned int bss_para_change_cnt_present:1;
	unsigned int medium_sync_delay_present:1;
	unsigned int eml_cap_present:1;
	unsigned int mld_cap_present:1;
	unsigned int mld_id_present:1;

	struct per_sta_profile profiles[ML_MAX_LINK_NUM];
};

/* common */
#ifdef CONFIG_MTK_IEEE80211BE
const u8 * ml_auth_spa(struct wpa_state_machine *sm, const u8 *own_addr);
const u8 * ml_auth_aa(struct wpa_state_machine *sm, const u8 *bssid);
#else
#define ml_auth_spa(__sm, __addr) __addr
#define ml_auth_aa(__sm, __addr) __addr
#endif
u8* ml_set_mac_kde(u8 *buf, const unsigned char *addr);
u8* ml_set_ml_link_kde(u8 *pos, u8 id, const unsigned char *addr,
	const u8 *rsne, size_t rsne_len, const u8 *rsnxe, size_t rsnxe_len);
int ml_parse_ie(const u8 *pos, size_t len, struct wpa_ml_ie_parse *ml, u8 *bssid, struct hostapd_data *hapd);

#ifdef CONFIG_SAE
int ml_sae_process_auth(struct sae_data *sae, u16 auth_transaction,
	const u8 *ies, size_t ies_len);
int ml_sae_write_auth(struct hostapd_data *hapd,
	struct sae_data *sae, struct wpabuf *buf);
#endif

/* AP */
void ml_identity_remove(struct wpa_state_machine *sm);
int ml_group_init(struct hostapd_data *hapd, u8 mld_grp_idx, u8 link_id, u8 *mld_addr);
int ml_group_deinit(struct hostapd_data *hapd);
int ml_group_deinit_for_reconfig(struct hostapd_data *hapd);
int ml_new_assoc_sta(struct wpa_state_machine *sm, const u8 *ie, size_t len);
u8* ml_add_m1_kde(struct wpa_state_machine *sm, u8 *pos);
int ml_process_m2_kde(struct wpa_state_machine *sm,
		const u8 *key_data, size_t key_data_len);
u8* ml_add_m3_kde(struct wpa_state_machine *sm, u8 *pos);
int ml_process_m4_kde(struct wpa_state_machine *sm,
		const u8 *key_data, size_t key_data_len);
u8* ml_set_gtk_kde(struct wpa_state_machine *sm, u8 *pos,
		   struct wpa_ml_link *link);
u8* ml_set_ieee80211w_kde(struct wpa_state_machine *sm, u8 *pos,
			  struct wpa_ml_link *link);
u8* ml_add_rekey_kde(struct wpa_state_machine *sm, u8 *pos);
int ml_rekey_gtk(struct wpa_state_machine *sm, struct wpa_eapol_ie_parse *kde);

#endif /* WPA_ML_COMMON_H */
