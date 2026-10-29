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
#include "includes.h"

#include "common.h"

#include "common/ieee802_11_defs.h"
#include "common/ieee802_11_common.h"
#include "common/sae.h"
#include "rsn_supp/wpa.h"
#include "rsn_supp/wpa_i.h"
#include "rsn_supp/wpa_ie.h"
#include "ap/wpa_auth.h"
#include "ap/wpa_auth_i.h"
#include "ap/wpa_auth_ie.h"
#include "ap/hostapd.h"
#include "crypto/random.h"
#include "utils/eloop.h"

#include "ml/ml_common.h"
#include "ml/ml_supplicant.h"

#include "wpa_supplicant_i.h"
#include "bss.h"
#include "driver_i.h" /* for drv cmd*/


#define CMD_PRESET_LINKID	"PRESET_LINKID"
static const u8 null_rsc[8] = { 0, 0, 0, 0, 0, 0, 0, 0 };


struct mtk_vendor_ie {
	u8 id;
	u8 len;
	u8 oui[3];
	u8 cap[4];
	u8 data[0];
};

struct mtk_pre_wifi7_ie {
	u8 id;
	u8 len;
	u8 version0;
	u8 version1;
	u8 data[0];
};

#define MTK_SYNERGY_CAP_SUPPORT_TLV		BIT(0)

#define MTK_OUI_ID_MLR				1
#define MTK_OUI_ID_PRE_WIFI7			2
#define MTK_OUI_ID_ICI				3
#define MTK_OUI_ID_CHIP_CAP			4


struct wpa_ie_parse {
	const u8 *ap_rsn_ie;
	const u8 *ap_rsnxe;
	size_t ap_rsn_ie_len;
	size_t ap_rsnxe_len;
};

struct ml_gtk_data {
	u8 link_id;
	enum wpa_alg alg;
	int tx, key_rsc_len, keyidx;
	u8 gtk[32];
	int gtk_len;
};


/* STA */

const u8 * ml_get_ie(const u8 *ies, size_t ie_len, u32 ml_ie_type)
{
	const struct element *elem;

/*
	for_each_element_extid(elem, WLAN_EID_EXT_MULTI_LINK, ies, ie_len) {
		if (ML_IS_CTRL_TYPE(&elem->id, ml_ie_type))
			return &elem->id;
	}
*/
	for_each_element(elem, ies, ie_len) {
		u8 id = elem->id, elen = elem->datalen;
		const u8 *pos = elem->data;

		if (id == WLAN_EID_EXTENSION &&
		    elen > 0 && pos[0] == WLAN_EID_EXT_MULTI_LINK) {
			if (ML_IS_CTRL_TYPE(&elem->id, ml_ie_type))
				return &elem->id;
		}

		if (id == WLAN_EID_VENDOR_SPECIFIC && elen > 7 &&
		    WPA_GET_BE24(pos) == OUI_MTK) {
			struct mtk_vendor_ie *ie = (struct mtk_vendor_ie *)elem;

			if (ie->cap[0] & MTK_SYNERGY_CAP_SUPPORT_TLV) {
				const struct element *sub_elem;
				const u8 *sub;
				size_t sub_len;

				sub = ie->data;
				sub_len = ie->len - 7;

				for_each_element_id(sub_elem, MTK_OUI_ID_PRE_WIFI7, sub, sub_len) {
					struct mtk_pre_wifi7_ie *pre7 = (struct mtk_pre_wifi7_ie *) sub_elem;
					const struct element *pre_elem;
					const u8 *pre;
					size_t pre_len;

					pre = pre7->data;
					pre_len = pre7->len - 2;

					for_each_element_extid(pre_elem, WLAN_EID_EXT_MULTI_LINK, pre, pre_len) {
						if (ML_IS_CTRL_TYPE(&pre_elem->id, ml_ie_type))
							return &pre_elem->id;
					}
				}
			}
		}
	}

	return NULL;
}

const u8 * ml_sm_spa(struct wpa_sm *sm, const u8 *own_addr)
{
	if(sm && own_addr && sm->dot11MultiLinkActivated) {
		if (os_memcmp(own_addr, sm->sta_ml_ie->ml_addr, ETH_ALEN) != 0) {
			wpa_printf(MSG_INFO,
				"ML: SPA[" MACSTR "]  use ml addr[" MACSTR "]",
				MAC2STR(own_addr), MAC2STR(sm->sta_ml_ie->ml_addr));
			return sm->sta_ml_ie->ml_addr;
		}
	}

	return own_addr;
}

const u8 * ml_sm_aa(struct wpa_sm *sm, const u8 *bssid)
{
	if(sm && bssid && sm->dot11MultiLinkActivated) {
		if (os_memcmp(bssid, sm->bssid, ETH_ALEN) == 0) {
			if (os_memcmp(bssid, sm->ap_ml_ie->ml_addr, ETH_ALEN) != 0) {
				wpa_printf(MSG_INFO,
					"ML: AA[" MACSTR "]  use ml addr[" MACSTR "]",
					MAC2STR(bssid), MAC2STR(sm->ap_ml_ie->ml_addr));
				return sm->ap_ml_ie->ml_addr;
			}
		} else {
			/* for preauth */
			struct wpa_supplicant *wpa_s = sm->ctx->ctx;
			struct wpa_bss *bss = wpa_bss_get_bssid_latest(wpa_s, bssid);

			if (bss && os_memcmp(bssid, bss->aa, ETH_ALEN) != 0) {
				wpa_printf(MSG_INFO,
					"ML: AA[" MACSTR "] use ml addr[" MACSTR "]",
					MAC2STR(bssid), MAC2STR(bss->aa));
				return bss->aa;
			}

		}
	}

	return bssid;
}

int ml_set_sae_auth_commit_req_ml_ie (struct sae_data *sae, const u8 *ies, size_t ies_len)
{
	struct ieee802_11_elems elems;
	struct wpa_ml_ie_parse ml;
	struct wpabuf *frag_ml_ie = NULL;
	const u8 *ml_ie;
	size_t ml_ie_len = 0;

	if (sae == NULL)
		return -1;

	if (ieee802_11_parse_elems(ies, ies_len, &elems, 1) == ParseFailed) {
		wpa_printf(MSG_DEBUG, "ML: Failed to parse elements");
		return -1;
	}

	frag_ml_ie = ieee802_11_defrag(&elems,
							 WLAN_EID_EXTENSION,
							 WLAN_EID_EXT_MULTI_LINK);
	if (!frag_ml_ie) {
		wpa_printf(MSG_DEBUG, "ML: Missing frag_ml_ie");
		ml_ie = elems.ml;
		ml_ie_len = (size_t)elems.ml_len;
	} else {
		ml_ie_len = (size_t)wpabuf_len(frag_ml_ie);
		ml_ie = wpabuf_head_u8(frag_ml_ie);

		wpa_hexdump(MSG_DEBUG, "Multi-link IE defrag results:",
			ml_ie, ml_ie_len);
	}
	os_memset(sae->own_ml_addr, 0, ETH_ALEN);
	if (!ml_ie) {
		wpa_printf(MSG_DEBUG, "ML: clearing STA ML IE");
		sae->dot11MultiLinkActivated = 0;
	} else {
		if (ml_parse_ie(ml_ie, ml_ie_len, &ml, NULL, NULL) != 0) {
			sae->dot11MultiLinkActivated = 0;
			if (frag_ml_ie)
				wpabuf_free(frag_ml_ie);
			return -1;
		} else {
			os_memcpy(sae->own_ml_addr, ml.ml_addr, ETH_ALEN);
			sae->dot11MultiLinkActivated = 1;
			wpa_printf(MSG_DEBUG, "(%s)[%d]ML:succcown_ml_addr: "MACSTR", dot11MultiLinkActivated: %u",
				__func__, __LINE__,MAC2STR(sae->own_ml_addr), sae->dot11MultiLinkActivated);
		}
	}
	if (frag_ml_ie)
		wpabuf_free(frag_ml_ie);
	wpa_printf(MSG_DEBUG, "(%s)[%d]ML:own_ml_addr: "MACSTR", dot11MultiLinkActivated: %u",
		__func__, __LINE__,MAC2STR(sae->own_ml_addr), sae->dot11MultiLinkActivated);
	return 0;
}



int ml_set_assoc_req_ml_ie(struct wpa_sm *sm, const u8 *ies, size_t ies_len)
{
	struct ieee802_11_elems elems;
	struct wpa_ml_ie_parse ml;
	struct wpabuf *frag_ml_ie = NULL;
	const u8 *ml_ie = NULL;
	size_t ml_ie_len = 0;

	if (sm == NULL)
		return -1;

	if (ieee802_11_parse_elems(ies, ies_len, &elems, 1) == ParseFailed) {
		wpa_printf(MSG_DEBUG, "ML: Failed to parse elements");
		return -1;
	}

	frag_ml_ie = ieee802_11_defrag(&elems,
						 WLAN_EID_EXTENSION,
						 WLAN_EID_EXT_MULTI_LINK);
	if (!frag_ml_ie) {
		wpa_printf(MSG_DEBUG, "ML: Missing frag_ml_ie");
		ml_ie = elems.ml;
		ml_ie_len = (size_t)elems.ml_len;
	} else {
		ml_ie_len = (size_t)wpabuf_len(frag_ml_ie);
		ml_ie = wpabuf_head_u8(frag_ml_ie);

		wpa_hexdump(MSG_DEBUG, "Multi-link IE defrag results:",
			ml_ie, ml_ie_len);
	}

	os_free(sm->sta_ml_ie);
	if (!ml_ie) {
		wpa_printf(MSG_DEBUG, "ML:[%s] clearing STA ML IE", __func__);
		goto err;
	} else {
		if (ml_parse_ie(ml_ie, ml_ie_len, &ml, NULL, NULL) != 0 ||
		    ml.prof_num > ML_MAX_LINK_NUM) {
			wpa_printf(MSG_DEBUG, "ML:[%s] ml_parse_ie fails", __func__);
			goto err;
		} else {
			sm->sta_ml_ie = os_memdup(&ml, sizeof(ml));
			if (sm->sta_ml_ie == NULL) {
				wpa_printf(MSG_DEBUG, "ML:[%s] sta_ml_ie is null.", __func__);
				goto err;
			}

			os_memcpy(sm->own_ml_addr, ml.ml_addr, ETH_ALEN);
			sm->prof_num = ml.prof_num;
			sm->dot11MultiLinkActivated = 1;
		}
	}
	if (frag_ml_ie)
		wpabuf_free(frag_ml_ie);
	return 0;

err:
	if (frag_ml_ie)
		wpabuf_free(frag_ml_ie);
	sm->sta_ml_ie = NULL;
	sm->prof_num = 0;
	sm->dot11MultiLinkActivated = 0;
	return -1;
}

int ml_set_assoc_resp_ml_ie(struct wpa_sm *sm, const u8 *ies, size_t ies_len, u8 *bssid)
{
	struct ieee802_11_elems elems;
	struct wpa_ml_ie_parse ml;
	struct wpabuf *frag_ml_ie = NULL;
	const u8 *ml_ie;
	size_t ml_ie_len = 0;

	if (sm == NULL)
		return -1;

	if (ieee802_11_parse_elems(ies, ies_len, &elems, 1) == ParseFailed) {
		wpa_printf(MSG_DEBUG, "ML: Failed to parse elements");
		return -1;
	}
	frag_ml_ie = ieee802_11_defrag(&elems,
					 WLAN_EID_EXTENSION,
					 WLAN_EID_EXT_MULTI_LINK);

	if (!frag_ml_ie) {
		wpa_printf(MSG_DEBUG, "ML: Missing frag_ml_ie");
		ml_ie = elems.ml;
		ml_ie_len = (size_t)elems.ml_len;
	} else {
		ml_ie_len = (size_t)wpabuf_len(frag_ml_ie);
		ml_ie = wpabuf_head_u8(frag_ml_ie);

		wpa_hexdump(MSG_DEBUG, "Multi-link IE defrag results:",
			ml_ie, ml_ie_len);
	}

	os_free(sm->ap_ml_ie);
	if (!ml_ie) {
		WPA_ASSERT(!sm->dot11MultiLinkActivated);

		wpa_dbg(sm->ctx->msg_ctx, MSG_DEBUG, "ML: clearing AP ML IE");
		goto err;
	} else {
		WPA_ASSERT(sm->dot11MultiLinkActivated);

		if (ml_parse_ie(ml_ie, ml_ie_len, &ml, bssid, NULL) != 0  ||
		    ml.prof_num > ML_MAX_LINK_NUM) {
			wpa_printf(MSG_DEBUG, "ML:[%s] ml_parse_ie fails", __func__);
			goto err;
		} else {
			WPA_ASSERT(ml.prof_num == sm->prof_num);

			sm->ap_ml_ie = os_memdup(&ml, sizeof(ml));
			if (sm->ap_ml_ie == NULL) {
				wpa_printf(MSG_DEBUG, "ML:[%s] ap_ml_ie is null.", __func__);
				goto err;
			}
			os_memcpy(sm->ml_bssid, ml.ml_addr, ETH_ALEN);
		}
	}
	if (frag_ml_ie)
		wpabuf_free(frag_ml_ie);
	wpa_printf(MSG_DEBUG, "(%s)[%d]:sm->dot11MultiLinkActivated: %u.\n",
		__func__, __LINE__, sm->dot11MultiLinkActivated);
	return 0;

err:
	if (frag_ml_ie)
		wpabuf_free(frag_ml_ie);
	if (sm->dot11MultiLinkActivated) {
		wpa_dbg(sm->ctx->msg_ctx, MSG_ERROR, "ML: clearing STA ML IE");
		if (sm->sta_ml_ie)
			os_free(sm->sta_ml_ie);
		sm->sta_ml_ie = NULL;
		sm->prof_num = 0;
		sm->dot11MultiLinkActivated = 0;
	}
	wpa_dbg(sm->ctx->msg_ctx, MSG_DEBUG, "ML: clearing AP ML IE");
	sm->ap_ml_ie = NULL;

	return -1;
}

size_t ml_add_m2_kde(struct wpa_sm *sm, u8 *pos)
{
	struct wpa_ml_ie_parse *ml = sm->sta_ml_ie;
	size_t i, count = 0;
	u8 *buf = pos;

	if (!sm->dot11MultiLinkActivated)
		return 0;

	wpa_printf(MSG_DEBUG, "ML: Add Mac/Link into EAPOL-Key 2/4");
	wpa_printf(MSG_DEBUG, "(%s)[%d] ML: Add Mac: "MACSTR"", __func__, __LINE__, MAC2STR(sm->own_ml_addr));
	pos = ml_set_mac_kde(pos, sm->own_ml_addr);

	for (i = 0; i < sm->prof_num; i++) {
		struct per_sta_profile *sta = &ml->profiles[i];

		/* normally this won't happen, just in case sta carries
		 * sta profile for main link and it's for single link setup
		 */
		if (sta->link_id == sm->ap_ml_ie->link_id)
			continue;
		count++;
	}

	/* single link doesn't mlo link kde */
	if (count) {
		wpa_printf(MSG_DEBUG, "ML: Add Link into EAPOL-Key 2/4");

		for (i = 0; i < sm->prof_num; i++) {
			struct per_sta_profile *sta = &ml->profiles[i];

			if (sta->link_id == sm->ap_ml_ie->link_id)
				continue;

			pos = ml_set_ml_link_kde(pos, sta->link_id, sta->addr,
				NULL, 0, NULL, 0);
		}
	}

	return pos - buf;
}

static int ml_get_wpa_ie(struct wpa_supplicant *wpa_s, u8 *bssid,
			 struct wpa_ie_parse *wpa)
{
	int ret = 0;
	struct wpa_bss *curr = NULL, *bss;
	const u8 *ie;

	wpa_printf(MSG_DEBUG, "searching for bssid: "MACSTR"", MAC2STR(bssid));
	dl_list_for_each(bss, &wpa_s->bss, struct wpa_bss, list) {
		wpa_printf(MSG_DEBUG, "scan table bss bssid: "MACSTR"", MAC2STR(bss->bssid));
		if (os_memcmp(bss->bssid, bssid, ETH_ALEN) != 0)
			continue;
		if (bss->ssid_len == 0) {
			wpa_printf(MSG_DEBUG, "skip for SSID len is zero means beacon entry hidden ssid");
			continue;
		}
		curr = bss;
		wpa_printf(MSG_DEBUG, "target bssid: "MACSTR"", MAC2STR(bssid));
		break;
	}

	if (!curr) {
		wpa_printf(MSG_DEBUG, "can't find the curr BSSID Add Mac: "MACSTR"", MAC2STR(bssid));
		return -1;
	}

	os_memset(wpa, 0, sizeof(*wpa));

	ie = wpa_bss_get_ie(curr, WLAN_EID_RSN);
	if (ie) {
		wpa->ap_rsn_ie = ie;
		wpa->ap_rsn_ie_len = 2 + ie[1];
	}

	ie = wpa_bss_get_ie(curr, WLAN_EID_RSNX);
	if (ie) {
		wpa->ap_rsnxe = ie;
		wpa->ap_rsnxe_len = 2 + ie[1];
	}

	return 0;
}

int ml_validate_m3_kde(struct wpa_sm *sm, const struct wpa_eapol_key *key,
	struct wpa_eapol_ie_parse *ie)
{
	u16 key_info;
	size_t i, j;
	u8 found = 0;
	struct wpa_ie_parse wpa, *target_wpa = NULL;
	struct wpa_mlo_link_kde *mlo_link = NULL;

	key_info = WPA_GET_BE16(key->key_info);

	if(!sm->dot11MultiLinkActivated) {
		if (ie->mlo_gtk.num == 0 && ie->mlo_igtk.num == 0 &&
		    ie->mlo_bigtk.num == 0 && ie->mlo_link.num == 0) {
			wpa_printf(MSG_INFO, "ML: EAPOL-Key 3/4 inactive");
			return 0;
		} else {
			wpa_printf(MSG_INFO, "ML: EAPOL-Key 3/4 inactive but "
				"with ml kde (gtk=%d, igtk=%d, bigtk=%d link=%d)",
				(int)ie->mlo_gtk.num, (int)ie->mlo_igtk.num,
				(int)ie->mlo_bigtk.num, (int)ie->mlo_link.num);
			return -1;
		}
	}

	/* mac addr */
	if (sm->ap_ml_ie &&
	    os_memcmp(sm->ap_ml_ie->ml_addr, ie->mac_addr, ETH_ALEN) != 0) {
		wpa_printf(MSG_INFO, "ML: EAPOL-Key 3/4 wrong ml addr");
		return -1;
	}


	if (ie->rsn_ie) {
		wpa_printf(MSG_INFO,
			"ML: EAPOL-Key 3/4 should not have Basic RSN IE");
		return -1;
	}

	if (ie->rsnxe) {
		wpa_printf(MSG_INFO,
			"ML: EAPOL-Key 3/4 should not have Basic RSNXE IE");
		return -1;
	}
	for (i = 0; i < ie->mlo_link.num; i++) {
		mlo_link = (struct wpa_mlo_link_kde *) ie->mlo_link.kdes[i].data;
		if (ml_get_wpa_ie(sm->ctx->ctx, mlo_link->addr, &wpa) < 0) {
			wpa_printf(MSG_ERROR, "ML: Could not find mlo_link("MACSTR") from the scan results", MAC2STR(mlo_link->addr));
		} else {
			target_wpa = &wpa;
			break;
		}
	}
	if (target_wpa == NULL) {
		wpa_printf(MSG_ERROR,
			"ML: can't find one of the mlo link ssid in the scan table.");
		return -1;
	}

	/* mlo link id & rsne & rsnxe */
	for (i = 0; i < ie->mlo_link.num; i++) {
		struct wpa_mlo_link_kde *mlo_link =
			(struct wpa_mlo_link_kde *) ie->mlo_link.kdes[i].data;
		size_t len = ie->mlo_link.kdes[i].len;
		u8 *rsne = NULL, *rsnxe = NULL;
		u8 rsne_len = 0, rsnxe_len = 0; /* including hdr */


		if (len < sizeof(struct wpa_mlo_link_kde)) {
			wpa_printf(MSG_INFO, "ML: EAPOL-Key 3/4 error mlo link");
			return -1;
		}

		len -= sizeof(struct wpa_mlo_link_kde);
		if (mlo_link->info & BIT(4)) {
			if (len < 2 || len < mlo_link->var[1] + 2) {
				wpa_printf(MSG_INFO, "ML: EAPOL-Key 3/4 wrong mlo rsne len");
				return -1;
			} else {
				rsne = &mlo_link->var[0];
				rsne_len = mlo_link->var[1] + 2;
				len -= rsne_len;
			}
		}

		if (mlo_link->info & BIT(5)) {
			if (len < 2 || len < mlo_link->var[rsne_len + 1] + 2) {
				wpa_printf(MSG_INFO, "ML: EAPOL-Key 3/4 wrong mlo rsnxe len");
				return -1;
			} else {
				rsnxe = &mlo_link->var[rsne_len];
				rsnxe_len = mlo_link->var[rsne_len + 1] + 2;
				len -= rsnxe_len;
			}
		}

		if (len != 0) {
			wpa_printf(MSG_INFO,
				"ML: EAPOL-Key 3/4 (%d/%d) link id=%d wrong data len, rsne_len=%d, rsnxe_len=%d, left=%d",
					(int)i, (int)ie->mlo_link.num, mlo_link->info & 0xf,
					rsne_len, rsnxe_len, (int)len);
			return -1;
		}
		/*mlo link kde*/
		if (sm->ap_ml_ie) {
			found = 0;
			for (j = 0; j < sm->ap_ml_ie->prof_num; j++) {
				if (os_memcmp(sm->ap_ml_ie->profiles[j].addr,
						mlo_link->addr, ETH_ALEN) == 0 &&
				    sm->ap_ml_ie->profiles[j].link_id ==
						(mlo_link->info & 0xf)) {
					found = 1;
					break;
				}
			}

			if (!found) {
				/*setup link kde not check*/
				if (os_memcmp(sm->bssid,
					mlo_link->addr, ETH_ALEN) != 0 ||
				    sm->ap_ml_ie->link_id != (mlo_link->info & 0xf)) {
					wpa_printf(MSG_INFO,
						"ML: EAPOL-Key 3/4 wrong link, expect["MACSTR", %u] input["MACSTR", %u]",
						MAC2STR(sm->bssid), sm->ap_ml_ie->link_id,
						MAC2STR(mlo_link->addr), mlo_link->info & 0xf);
					continue;
				}
			}
		}

		/* mlo without rsn/rsx but beacon does or length not matched */
		if ((!(mlo_link->info & 0xf0) && (target_wpa->ap_rsn_ie || target_wpa->ap_rsnxe))) {
			wpa_printf(MSG_INFO, "ML: IE in 3/4 msg does not match "
					     "with IE in Beacon/ProbeResp (no IE?)");
			return -1;
		}

		/* rsne */ /* mrsno case not check rsne now, if need, TBD */
		if (sm->rsn_override == RSN_OVERRIDE_NOT_USED && rsne && target_wpa->ap_rsn_ie &&
		    wpa_compare_rsn_ie(wpa_key_mgmt_ft(sm->key_mgmt),
					target_wpa->ap_rsn_ie, target_wpa->ap_rsn_ie_len,
					rsne, rsne_len)) {
			wpa_printf(MSG_INFO, "ML: IE in 3/4 msg does not match "
					     "with IE in Beacon/ProbeResp (rsne)");
			wpa_hexdump(MSG_INFO, "RSNE in Beacon/ProbeResp",
				    target_wpa->ap_rsn_ie, target_wpa->ap_rsn_ie_len);
			wpa_hexdump(MSG_INFO, "RSNE in EAPOL-Key msg 3/4",
				    rsne, rsne_len);
			{
				size_t ie1len=  target_wpa->ap_rsn_ie_len, ie2len = rsne_len;
				struct wpa_ie_data ie1d, ie2d;
				if (wpa_parse_wpa_ie_rsn(target_wpa->ap_rsn_ie, ie1len, &ie1d) < 0 ||
				    wpa_parse_wpa_ie_rsn(rsne, ie2len, &ie2d) < 0)
			return -1;
				wpa_printf(MSG_INFO, "ML: IE in 3/4 msg GTK/PTK COUNTER: 0x%04x- 0x%04x",
					     ie1d.capabilities, ie2d.capabilities);
				if (ie1d.proto == ie2d.proto &&
				    ie1d.pairwise_cipher == ie2d.pairwise_cipher &&
				    ie1d.group_cipher == ie2d.group_cipher &&
				    (ie1d.key_mgmt & ie2d.key_mgmt) &&
				    ((ie1d.capabilities & 0xffbf) == (ie2d.capabilities & 0xffbf))&&
				    ie1d.mgmt_group_cipher == ie2d.mgmt_group_cipher)
					/*
				    spec draft2.0
					12.6.2 RSNA selection
					Insert the following paragraph after the third paragraph ("A STA shall advertise the same
					RSNE..."):
					All APs affiliated with an AP MLD shall advertise the same RSNE and RSNXE if included, with the
					exception of the AKM Suite List field and the MFPR subfield of the RSN Capabilities field. All APs
					affiliated with an AP MLD shall advertise at least one common AKM suite selector in the AKM Suite List
					field.
					*/
					wpa_printf(MSG_INFO, "ML: PASS: IE in 3/4 msg RSN AKM have common suite or MFPR not same."
					     "with IE in Beacon/ProbeResp (rsne), check pass.");
				else
					return -1;
			}
		}

		if (sm->proto == WPA_PROTO_WPA &&
		    rsne && target_wpa->ap_rsn_ie == NULL && sm->rsn_enabled) {
			wpa_printf(MSG_INFO, "ML: Possible downgrade attack "
					       "detected - RSN was enabled and RSN IE "
					       "was in msg 3/4, but not in "
					       "Beacon/ProbeResp");
			return -1;
		}

		/* mrsno case not check rsnxe now, if need, TBD */
		if (sm->rsn_override == RSN_OVERRIDE_NOT_USED && sm->proto == WPA_PROTO_RSN &&
		    ((target_wpa->ap_rsnxe && !rsnxe) ||
		     (!target_wpa->ap_rsnxe && rsnxe) ||
		     (target_wpa->ap_rsnxe && rsnxe &&
		      (target_wpa->ap_rsnxe_len != rsnxe_len ||
		       os_memcmp(target_wpa->ap_rsnxe, rsnxe, target_wpa->ap_rsnxe_len) != 0)))) {
			wpa_printf(MSG_INFO, "ML: RSNXE mismatch between Beacon/ProbeResp and EAPOL-Key msg 3/4");
			wpa_hexdump(MSG_INFO, "RSNXE in Beacon/ProbeResp",
				    target_wpa->ap_rsnxe, target_wpa->ap_rsnxe_len);
			wpa_hexdump(MSG_INFO, "RSNXE in EAPOL-Key msg 3/4",
				    rsnxe, rsnxe_len);
			return -1;
		}
	}

	/* mlo gtk */
	if (ie->gtk) {
		wpa_printf(MSG_INFO,
			"ML: EAPOL-Key 3/4 should not have Basic GTK IE");
		return -1;
	}

	if (ie->mlo_gtk.num > 0 && !(key_info & WPA_KEY_INFO_ENCR_KEY_DATA)) {
		wpa_printf(MSG_INFO,
			"ML: EAPOL-Key 3/4 MLO GTK IE in unencrypted key data");
		return -1;
	}


	for (i = 0; i < ie->mlo_gtk.num; i++) {
		struct wpa_mlo_gtk_kde *mlo_gtk =
			(struct wpa_mlo_gtk_kde *) ie->mlo_gtk.kdes[i].data;
		u8 link_id = (mlo_gtk->info & 0xf0) >> 4;

		if (sm->ap_ml_ie) {
			found = 0;
			for (j = 0; j < sm->ap_ml_ie->prof_num; j++) {
				if (link_id == sm->ap_ml_ie->profiles[j].link_id) {
					found = 1;
					break;
				}
			}
			if (!found) {
				if (link_id != sm->ap_ml_ie->link_id) {
					wpa_printf(MSG_INFO, "ML: EAPOL-Key 3/4 wrong gtk link id, expect=%u input=%u",
						   sm->ap_ml_ie->link_id, link_id);
					return -1;
				}
			}
		}
	}


	/* mlo igtk */
	if (ie->igtk) {
		wpa_printf(MSG_INFO,
			"ML: EAPOL-Key 3/4 should not have Basic IGTK IE");
		return -1;
	}

	if (ie->mlo_igtk.num > 0 && !(key_info & WPA_KEY_INFO_ENCR_KEY_DATA)) {
		wpa_printf(MSG_INFO,
			"ML: EAPOL-Key 3/4 MLO IGTK IE in unencrypted key data");
		return -1;
	}

	for (i = 0; i < ie->mlo_igtk.num; i++) {
		struct wpa_mlo_igtk_kde *mlo_igtk =
			(struct wpa_mlo_igtk_kde *) ie->mlo_igtk.kdes[i].data;
		u8 link_id = (mlo_igtk->info & 0xf0) >> 4;
		size_t len = ie->mlo_igtk.kdes[i].len;

		if (sm->ap_ml_ie) {
			found = 0;
			for (j = 0; j < sm->ap_ml_ie->prof_num; j++) {
				if (link_id == sm->ap_ml_ie->profiles[j].link_id) {
					found = 1;
					break;
				}
			}
			if (!found) {
				if (link_id != sm->ap_ml_ie->link_id) {
					wpa_printf(MSG_INFO, "ML: EAPOL-Key 3/4 wrong igtk link id, expect=%u input=%u",
						   sm->ap_ml_ie->link_id, link_id);
					return -1;
				}
			}
		}

		if (sm->mgmt_group_cipher != WPA_CIPHER_GTK_NOT_USED &&
		    wpa_cipher_valid_mgmt_group(sm->mgmt_group_cipher) &&
		    len != WPA_MLO_IGTK_KDE_PREFIX_LEN +
		    (unsigned int) wpa_cipher_key_len(sm->mgmt_group_cipher)) {
			wpa_printf(MSG_INFO, "ML: Invalid IGTK KDE length %lu",
				(unsigned long) len);
			return -1;
		}
	}

	/* mlo bigtk */
	if (ie->bigtk) {
		wpa_printf(MSG_INFO,
			"ML: EAPOL-Key 3/4 should not have Basic BIGTK IE");
		return -1;
	}

	for (i = 0; i < ie->mlo_bigtk.num; i++) {
		struct wpa_mlo_bigtk_kde *mlo_bigtk =
			(struct wpa_mlo_bigtk_kde *) ie->mlo_bigtk.kdes[i].data;
		u8 link_id = (mlo_bigtk->info & 0xf0) >> 4;
		size_t len = ie->mlo_bigtk.kdes[i].len;

		if (sm->ap_ml_ie) {
			found = 0;
			for (j = 0; j < sm->ap_ml_ie->prof_num; j++) {
				if (link_id == sm->ap_ml_ie->profiles[j].link_id) {
					found = 1;
					break;
				}
			}
			if (!found) {
				if (link_id != sm->ap_ml_ie->link_id) {
					wpa_printf(MSG_INFO, "ML: EAPOL-Key 3/4 wrong bigtk link id, expect=%u input=%u",
						   sm->ap_ml_ie->link_id, link_id);
					return -1;
				}
			}
		}
		if (sm->mgmt_group_cipher != WPA_CIPHER_GTK_NOT_USED &&
		    wpa_cipher_valid_mgmt_group(sm->mgmt_group_cipher) &&
		    len != WPA_MLO_BIGTK_KDE_PREFIX_LEN +
		    (unsigned int) wpa_cipher_key_len(sm->mgmt_group_cipher)) {
			wpa_printf(MSG_INFO, "ML: Invalid BIGTK KDE length %lu",
				(unsigned long) len);
			return -1;
		}
	}

	return 0;
}

static int ml_rsc_relaxation(const struct wpa_sm *sm, const u8 *rsc)
{
	int rsclen;

	if (!sm->wpa_rsc_relaxation)
		return 0;

	rsclen = wpa_cipher_rsc_len(sm->group_cipher);

	/*
	 * Try to detect RSC (endian) corruption issue where the AP sends
	 * the RSC bytes in EAPOL-Key message in the wrong order, both if
	 * it's actually a 6-byte field (as it should be) and if it treats
	 * it as an 8-byte field.
	 * An AP model known to have this bug is the Sapido RB-1632.
	 */
	if (rsclen == 6 && ((rsc[5] && !rsc[0]) || rsc[6] || rsc[7])) {
		wpa_msg(sm->ctx->msg_ctx, MSG_WARNING,
			"RSC %02x%02x%02x%02x%02x%02x%02x%02x is likely bogus, using 0",
			rsc[0], rsc[1], rsc[2], rsc[3],
			rsc[4], rsc[5], rsc[6], rsc[7]);

		return 1;
	}

	return 0;
}


static int ml_gtk_tx_bit_workaround(const struct wpa_sm *sm,
						int tx)
{
	if (tx && sm->pairwise_cipher != WPA_CIPHER_NONE) {
		/* Ignore Tx bit for GTK if a pairwise key is used. One AP
		 * seemed to set this bit (incorrectly, since Tx is only when
		 * doing Group Key only APs) and without this workaround, the
		 * data connection does not work because wpa_supplicant
		 * configured non-zero keyidx to be used for unicast. */
		wpa_msg(sm->ctx->msg_ctx, MSG_INFO,
			"ML: Tx bit set for GTK, but pairwise "
			"keys are used - ignore Tx bit");
		return 0;
	}
	return tx;
}

static int ml_check_group_cipher(struct wpa_sm *sm,
					     int group_cipher,
					     int keylen, int maxkeylen,
					     int *key_rsc_len,
					     enum wpa_alg *alg)
{
	int klen;

	*alg = wpa_cipher_to_alg(group_cipher);
	if (*alg == WPA_ALG_NONE) {
		wpa_msg(sm->ctx->msg_ctx, MSG_WARNING,
			"ML: Unsupported Group Cipher %d",
			group_cipher);
		return -1;
	}
	*key_rsc_len = wpa_cipher_rsc_len(group_cipher);

	klen = wpa_cipher_key_len(group_cipher);
	if (keylen != klen || maxkeylen < klen) {
		wpa_msg(sm->ctx->msg_ctx, MSG_WARNING,
			"ML: Unsupported %s Group Cipher key length %d (%d)",
			wpa_cipher_txt(group_cipher), keylen, maxkeylen);
		return -1;
	}
	return 0;
}

static int ml_install_gtk(struct wpa_sm *sm,
			const struct wpa_eapol_key *key,
			struct wpa_eapol_ie_parse *ie, u8 wnm_sleep)
{
	struct wpa_supplicant *wpa_s = sm->ctx->ctx;
	struct ml_gtk_data data, *gd = &data;
	const u8 *key_rsc, *gtk;
	size_t gtk_len, i;
	char cmd[32], buf[256];
	u8 gtk_buf[32], *_gtk;

	for (i = 0; i < ie->mlo_gtk.num; i++) {
		gtk = ie->mlo_gtk.kdes[i].data;
		gtk_len = ie->mlo_gtk.kdes[i].len;

		os_memset(gd, 0, sizeof(*gd));
		wpa_hexdump_key(MSG_DEBUG, "ML: received GTK in pairwise handshake",
				gtk, gtk_len);

		if (gtk_len < WPA_MLO_GTK_KDE_PREFIX_LEN ||
		    gtk_len - WPA_MLO_GTK_KDE_PREFIX_LEN > sizeof(gd->gtk))
			return -1;

		gd->link_id = (gtk[0] & 0xf0) >> 4;
		gd->keyidx = gtk[0] & 0x3;
		gd->tx = ml_gtk_tx_bit_workaround(sm, !!(gtk[0] & BIT(2)));
		gtk += WPA_MLO_GTK_KDE_PREFIX_LEN ;
		gtk_len -= WPA_MLO_GTK_KDE_PREFIX_LEN;

		os_memcpy(gd->gtk, gtk, gtk_len);
		gd->gtk_len = gtk_len;

		key_rsc = key->key_rsc;
		if (ml_rsc_relaxation(sm, key->key_rsc))
			key_rsc = null_rsc;


		if (ml_check_group_cipher(sm, sm->group_cipher,
				       gtk_len, gtk_len,
				       &gd->key_rsc_len, &gd->alg)) {
			wpa_dbg(sm->ctx->msg_ctx, MSG_DEBUG,
				"ML: Check group cipher failed");
			forced_memzero(gd, sizeof(*gd));
			return -1;
		}

		_gtk = gd->gtk;

		/* Detect possible key reinstallation */
		if ((sm->ml_gtk.gtks[i].gtk_len == (size_t) gd->gtk_len &&
		     os_memcmp(sm->ml_gtk.gtks[i].gtk, gd->gtk, sm->ml_gtk.gtks[i].gtk_len) == 0) ||
		    (sm->ml_gtk_wnm_sleep.gtks[i].gtk_len == (size_t) gd->gtk_len &&
		     os_memcmp(sm->ml_gtk_wnm_sleep.gtks[i].gtk, gd->gtk,
			       sm->ml_gtk_wnm_sleep.gtks[i].gtk_len) == 0)) {
			wpa_dbg(sm->ctx->msg_ctx, MSG_DEBUG,
				"ML: Not reinstalling already in-use GTK to the driver (keyidx=%d tx=%d len=%d)",
				gd->keyidx, gd->tx, gd->gtk_len);
			continue;
		}

		wpa_hexdump_key(MSG_INFO, "ML: Group Key", gd->gtk, gd->gtk_len);
		wpa_dbg(sm->ctx->msg_ctx, MSG_INFO,
			"ML: Installing GTK to the driver (keyidx=%d tx=%d len=%d)",
			gd->keyidx, gd->tx, gd->gtk_len);
		wpa_hexdump(MSG_INFO, "WPA: RSC", key_rsc, gd->key_rsc_len);
		if (sm->group_cipher == WPA_CIPHER_TKIP) {
			/* Swap Tx/Rx keys for Michael MIC */
			os_memcpy(gtk_buf, gd->gtk, 16);
			os_memcpy(gtk_buf + 16, gd->gtk + 24, 8);
			os_memcpy(gtk_buf + 24, gd->gtk + 16, 8);
			_gtk = gtk_buf;
		}

		// TODO: remove this when kernel is ready
		os_snprintf(cmd, sizeof(cmd), CMD_PRESET_LINKID "=%u", gd->link_id);
		wpa_drv_driver_cmd(wpa_s, cmd, buf, sizeof(buf));

		if (sm->pairwise_cipher == WPA_CIPHER_NONE) {
			if (wpa_sm_set_key(sm, gd->alg, NULL,
					   gd->keyidx, 1, key_rsc, gd->key_rsc_len,
					   _gtk, gd->gtk_len,
					   KEY_FLAG_GROUP_RX_TX_DEFAULT) < 0) {
				wpa_msg(sm->ctx->msg_ctx, MSG_WARNING,
					"ML: Failed to set GTK to the driver "
					"(Group only)");
				forced_memzero(gtk_buf, sizeof(gtk_buf));
				return -1;
			}
		} else if (wpa_sm_set_key(sm, gd->alg, broadcast_ether_addr,
					  gd->keyidx, gd->tx, key_rsc, gd->key_rsc_len,
					  _gtk, gd->gtk_len, KEY_FLAG_GROUP_RX) < 0) {
			wpa_msg(sm->ctx->msg_ctx, MSG_WARNING,
				"ML: Failed to set GTK to "
				"the driver (alg=%d keylen=%d keyidx=%d)",
				gd->alg, gd->gtk_len, gd->keyidx);
			forced_memzero(gtk_buf, sizeof(gtk_buf));
			return -1;
		}

		if (wnm_sleep) {
			sm->ml_gtk_wnm_sleep.gtks[i].gtk_len = gd->gtk_len;
			os_memcpy(sm->ml_gtk_wnm_sleep.gtks[i].gtk, gd->gtk,
				  sm->ml_gtk_wnm_sleep.gtks[i].gtk_len);
		} else {
			sm->ml_gtk.gtks[i].gtk_len = gd->gtk_len;
			os_memcpy(sm->ml_gtk.gtks[i].gtk, gd->gtk,
				  sm->ml_gtk.gtks[i].gtk_len);
		}

		forced_memzero(gd, sizeof(*gd));
		forced_memzero(gtk_buf, sizeof(gtk_buf));
	}

	return 0;
}

static int ml_install_igtk(struct wpa_sm *sm, const struct wpa_eapol_key *key,
		struct wpa_eapol_ie_parse *ie, u8 wnm_sleep)
{
	struct wpa_supplicant *wpa_s = sm->ctx->ctx;
	char cmd[32], buf[256];
	size_t i;
	size_t len = wpa_cipher_key_len(sm->mgmt_group_cipher);
	struct wpa_mlo_igtk_kde *igtk;
	size_t igtk_len;
	u16 keyidx;
	u8 link_id;

	for (i = 0; i < ie->mlo_igtk.num; i++) {
		igtk = (struct wpa_mlo_igtk_kde *) ie->mlo_igtk.kdes[i].data;
		igtk_len = ie->mlo_igtk.kdes[i].len;
		keyidx = WPA_GET_LE16(igtk->keyid);
		link_id = (igtk->info & 0xf0) >> 4;

		if (igtk_len != WPA_MLO_IGTK_KDE_PREFIX_LEN + len)
			return -1;

		/* Detect possible key reinstallation */
		if ((sm->ml_igtk.igtks[i].igtk_len == len &&
		     os_memcmp(sm->ml_igtk.igtks[i].igtk, igtk->igtk,
			       sm->ml_igtk.igtks[i].igtk_len) == 0) ||
		    (sm->ml_igtk_wnm_sleep.igtks[i].igtk_len == len &&
		     os_memcmp(sm->ml_igtk_wnm_sleep.igtks[i].igtk, igtk->igtk,
			       sm->ml_igtk_wnm_sleep.igtks[i].igtk_len) == 0)){
			wpa_dbg(sm->ctx->msg_ctx, MSG_DEBUG,
				"ML: Not reinstalling already in-use IGTK to the driver (keyidx=%d)",
				keyidx);
			continue;
		}

		wpa_dbg(sm->ctx->msg_ctx, MSG_INFO,
			"ML: IGTK keyid %d pn " COMPACT_MACSTR,
			keyidx, MAC2STR(igtk->pn));
		wpa_hexdump_key(MSG_DEBUG, "ML: IGTK", igtk->igtk, len);
		if (keyidx > 4095) {
			wpa_msg(sm->ctx->msg_ctx, MSG_WARNING,
				"ML: Invalid IGTK KeyID %d", keyidx);
			return -1;
		}


		// TODO: remove this when kernel is ready
		os_snprintf(cmd, sizeof(cmd), CMD_PRESET_LINKID "=%u", link_id);
		wpa_drv_driver_cmd(wpa_s, cmd, buf, sizeof(buf));

		if (wpa_sm_set_key(sm, wpa_cipher_to_alg(sm->mgmt_group_cipher),
				   broadcast_ether_addr,
				   keyidx, 0, igtk->pn, sizeof(igtk->pn),
				   igtk->igtk, len, KEY_FLAG_GROUP_RX) < 0) {
			if (keyidx == 0x0400 || keyidx == 0x0500) {
				/* Assume the AP has broken PMF implementation since it
				 * seems to have swapped the KeyID bytes. The AP cannot
				 * be trusted to implement BIP correctly or provide a
				 * valid IGTK, so do not try to configure this key with
				 * swapped KeyID bytes. Instead, continue without
				 * configuring the IGTK so that the driver can drop any
				 * received group-addressed robust management frames due
				 * to missing keys.
				 *
				 * Normally, this error behavior would result in us
				 * disconnecting, but there are number of deployed APs
				 * with this broken behavior, so as an interoperability
				 * workaround, allow the connection to proceed. */
				wpa_msg(sm->ctx->msg_ctx, MSG_INFO,
					"ML: Ignore IGTK configuration error due to invalid IGTK KeyID byte order");
			} else {
				wpa_msg(sm->ctx->msg_ctx, MSG_WARNING,
					"ML: Failed to configure IGTK to the driver");
				return -1;
			}
		}

		if (wnm_sleep) {
			sm->ml_igtk_wnm_sleep.igtks[i].igtk_len = len;
			os_memcpy(sm->ml_igtk_wnm_sleep.igtks[i].igtk, igtk->igtk, len);
		} else {
			sm->ml_igtk.igtks[i].igtk_len = len;
			os_memcpy(sm->ml_igtk.igtks[i].igtk, igtk->igtk, len);
		}
	}

	return 0;
}

static int ml_install_bigtk(struct wpa_sm *sm, const struct wpa_eapol_key *key,
		struct wpa_eapol_ie_parse *ie, u8 wnm_sleep)
{
	struct wpa_supplicant *wpa_s = sm->ctx->ctx;
	char cmd[32], buf[256];
	size_t i;
	size_t len = wpa_cipher_key_len(sm->mgmt_group_cipher);
	struct wpa_mlo_bigtk_kde *bigtk;
	size_t bigtk_len;
	u16 keyidx;
	u8 link_id;

	for (i = 0; i < ie->mlo_bigtk.num; i++) {
		bigtk = (struct wpa_mlo_bigtk_kde *) ie->mlo_bigtk.kdes[i].data;
		bigtk_len = ie->mlo_igtk.kdes[i].len;
		keyidx = WPA_GET_LE16(bigtk->keyid);
		link_id = (bigtk->info & 0xf0) >> 4;

		if (bigtk_len != WPA_MLO_BIGTK_KDE_PREFIX_LEN + len)
			return -1;

		/* Detect possible key reinstallation */
		if ((sm->ml_bigtk.bigtks[i].bigtk_len == len &&
		     os_memcmp(sm->ml_bigtk.bigtks[i].bigtk, bigtk->bigtk,
			       sm->ml_bigtk.bigtks[i].bigtk_len) == 0) ||
		    (sm->ml_bigtk_wnm_sleep.bigtks[i].bigtk_len == len &&
		     os_memcmp(sm->ml_bigtk_wnm_sleep.bigtks[i].bigtk, bigtk->bigtk,
			       sm->ml_bigtk_wnm_sleep.bigtks[i].bigtk_len) == 0)) {
			wpa_dbg(sm->ctx->msg_ctx, MSG_DEBUG,
				"ML: Not reinstalling already in-use BIGTK to the driver (keyidx=%d)",
				keyidx);
			return  0;
		}

		wpa_dbg(sm->ctx->msg_ctx, MSG_DEBUG,
			"ML: BIGTK keyid %d pn " COMPACT_MACSTR,
			keyidx, MAC2STR(bigtk->pn));
		wpa_hexdump_key(MSG_DEBUG, "ML: BIGTK", bigtk->bigtk, len);
		if (keyidx < 6 || keyidx > 7) {
			wpa_msg(sm->ctx->msg_ctx, MSG_WARNING,
				"ML: Invalid BIGTK KeyID %d", keyidx);
			return -1;
		}

		// TODO: remove this when kernel is ready
		os_snprintf(cmd, sizeof(cmd), CMD_PRESET_LINKID "=%u", link_id);
		wpa_drv_driver_cmd(wpa_s, cmd, buf, sizeof(buf));

		if (wpa_sm_set_key(sm, wpa_cipher_to_alg(sm->mgmt_group_cipher),
				   broadcast_ether_addr,
				   keyidx, 0, bigtk->pn, sizeof(bigtk->pn),
				   bigtk->bigtk, len, KEY_FLAG_GROUP_RX) < 0) {
			wpa_msg(sm->ctx->msg_ctx, MSG_WARNING,
				"WPA: Failed to configure BIGTK to the driver");
			return -1;
		}

		if (wnm_sleep) {
			sm->ml_bigtk_wnm_sleep.bigtks[i].bigtk_len = len;
			os_memcpy(sm->ml_bigtk_wnm_sleep.bigtks[i].bigtk, bigtk->bigtk,
				  sm->ml_bigtk_wnm_sleep.bigtks[i].bigtk_len);
		} else {
			sm->ml_bigtk.bigtks[i].bigtk_len = len;
			os_memcpy(sm->ml_bigtk.bigtks[i].bigtk, bigtk->bigtk,
				  sm->ml_bigtk.bigtks[i].bigtk_len);
		}
	}

	return 0;
}

int ml_process_m1_kde(struct wpa_sm *sm, struct wpa_eapol_ie_parse *ie)
{
	wpa_dbg(sm->ctx->msg_ctx, MSG_INFO, "(%s)[%d]:sm->dot11MultiLinkActivated: %u. MSG1\n",
				__func__, __LINE__, sm->dot11MultiLinkActivated);
	if (sm->dot11MultiLinkActivated) {
		if (ie->mac_addr) {
			wpa_hexdump(MSG_DEBUG, "ML: MAC from "
			    "Authenticator", ie->mac_addr, ie->mac_addr_len);
			if (os_memcmp(ie->mac_addr, sm->ml_bssid, ETH_ALEN) != 0) {
				wpa_dbg(sm->ctx->msg_ctx, MSG_ERROR,
				"ML: ML MAC Addr from M1 is different");
				return -1;
			}
		} else {
			wpa_dbg(sm->ctx->msg_ctx, MSG_ERROR,
				"ML: ML MAC Addr should be in M1");
			return -1;
		}
	}
	return 0;
}

int ml_process_m3_kde(struct wpa_sm *sm, const struct wpa_eapol_key *key,
	struct wpa_eapol_ie_parse *ie)
{
	u16 key_info;
	size_t i, j;
	u8 found = 0;

	key_info = WPA_GET_BE16(key->key_info);

	if(!sm->dot11MultiLinkActivated)
		return 0;

	/* mlo gtk */
	if (sm->group_cipher == WPA_CIPHER_GTK_NOT_USED) {
		/* No GTK to be set to the driver */
	} else if (ie->mlo_gtk.num > 0 &&
		ml_install_gtk(sm, key, ie, 0) < 0) {
		    wpa_msg(sm->ctx->msg_ctx, MSG_INFO,
			    "ML: Failed to configure GTK");
		return -1;
	}

	if (!wpa_cipher_valid_mgmt_group(sm->mgmt_group_cipher) ||
	    sm->mgmt_group_cipher == WPA_CIPHER_GTK_NOT_USED) {
		/* No IGTK to be set to the driver */
	} else if (ie->mlo_igtk.num > 0 &&
		ml_install_igtk(sm, key, ie, 0) < 0) {
		    wpa_msg(sm->ctx->msg_ctx, MSG_INFO,
			    "ML: Failed to configure IGTK");
		return -1;
	}

	if (!sm->beacon_prot) {
		/* No BIGTK to be set to the driver */
	} else if (ie->mlo_bigtk.num > 0 &&
		ml_install_bigtk(sm, key, ie, 0) < 0) {
		    wpa_msg(sm->ctx->msg_ctx, MSG_INFO,
			    "ML: Failed to configure BIGTK");
		return -1;
	}

	return 0;
}

int ml_process_1_of_2(struct wpa_sm *sm, const struct wpa_eapol_key *key,
	const u8 *key_data, size_t key_data_len, u16 key_info)
{
	struct wpa_eapol_ie_parse parse;
	struct wpa_eapol_ie_parse *ie = &parse;

	if(!sm->dot11MultiLinkActivated)
		return 0;

	wpa_hexdump(MSG_INFO, "ML: Group 1/2 IE KeyData", key_data, key_data_len);
	if (wpa_supplicant_parse_ies(key_data, key_data_len, ie) < 0)
		return -1;

	/* mlo gtk */
	if (sm->group_cipher == WPA_CIPHER_GTK_NOT_USED) {
		/* No GTK to be set to the driver */
	} else if (ie->mlo_gtk.num > 0 &&
		ml_install_gtk(sm, key, ie, 0) < 0) {
		    wpa_msg(sm->ctx->msg_ctx, MSG_INFO,
			    "ML: Failed to configure GTK");
		return -1;
	}

	if (!wpa_cipher_valid_mgmt_group(sm->mgmt_group_cipher) ||
	    sm->mgmt_group_cipher == WPA_CIPHER_GTK_NOT_USED) {
		/* No IGTK to be set to the driver */
	} else if (ie->mlo_igtk.num > 0 &&
		ml_install_igtk(sm, key, ie, 0) < 0) {
		    wpa_msg(sm->ctx->msg_ctx, MSG_INFO,
			    "ML: Failed to configure IGTK");
		return -1;
	}

	if (!sm->beacon_prot) {
		/* No BIGTK to be set to the driver */
	} else if (ie->mlo_bigtk.num > 0 &&
		ml_install_bigtk(sm, key, ie, 0) < 0) {
		    wpa_msg(sm->ctx->msg_ctx, MSG_INFO,
			    "ML: Failed to configure BIGTK");
		return -1;
	}

	return 0;
}

size_t ml_add_key_request_kde(struct wpa_sm *sm, u8 *pos)
{
	struct wpa_ml_ie_parse *ml = sm->sta_ml_ie;
	u8 i;

	if (!sm->dot11MultiLinkActivated)
		return 0;

	wpa_printf(MSG_DEBUG, "ML: Add Mac into Key Request");
	return ml_set_mac_kde(pos, sm->own_ml_addr) - pos;
}

size_t ml_add_m4_kde(struct wpa_sm *sm, u8 *pos)
{
	struct wpa_ml_ie_parse *ml = sm->sta_ml_ie;
	u8 i;

	if (!sm->dot11MultiLinkActivated)
		return 0;

	wpa_printf(MSG_DEBUG, "ML: Add Mac into EAPOL-Key 4/4");
	return ml_set_mac_kde(pos, sm->own_ml_addr) - pos;
}

size_t ml_add_2_of_2_kde(struct wpa_sm *sm, u8 *pos)
{
	struct wpa_ml_ie_parse *ml = sm->sta_ml_ie;
	u8 i;

	if (!sm->dot11MultiLinkActivated)
		return 0;

	wpa_printf(MSG_DEBUG, "ML: Add Mac into EAPOL-Key Group 2/2");
	return ml_set_mac_kde(pos, sm->own_ml_addr) - pos;
}

	/*
	 * First, determine the number of P2P supported channels in the
	 * pref_freq_list returned from driver. This is needed for calculations
	 * of the vendor IE size.
	 */





