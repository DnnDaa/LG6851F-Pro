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
#include <sys/stat.h>

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


#define STATE_MACHINE_ML_GROUP \
	(((struct hostapd_data *)sm->wpa_auth->cb_ctx)->ml_group)
#define STATE_MACHINE_ML_GROUP_ADDR \
	(((struct hostapd_data *)sm->wpa_auth->cb_ctx)->ml_group)->ml_addr

/* common */


u8* ml_set_mac_kde(u8 *pos, const unsigned char *addr)
{
	if (addr == NULL)
		return pos;

	return wpa_add_kde(pos, RSN_KEY_DATA_MAC_ADDR, addr, ETH_ALEN, NULL, 0);
}

u8* ml_set_ml_link_kde(u8 *pos, u8 link_id, const unsigned char *addr,
	const u8 *rsne, size_t rsne_len, const u8 *rsnxe, size_t rsnxe_len)
{
	u8 i;
	u8 *buf, *cp, *ori;
	size_t len = 1 /* Link Information */ + ETH_ALEN + rsne_len + rsnxe_len;

	cp = buf = os_malloc(len);
	if (!cp) {
		wpa_printf(MSG_ERROR, "ML: error cp is null, alloc fail, len:%zu", len);
		return pos;
	}
	os_memset(cp, 0, len);
	*cp = link_id & BITS(0, 3);
	if (rsne && rsne_len)
		*cp |= BIT(4);
	if (rsnxe && rsnxe_len)
		*cp |= BIT(5);
	cp++;
	os_memcpy(cp, addr, ETH_ALEN);
	cp += ETH_ALEN;

	if (rsne && rsne_len) {
		os_memcpy(cp, rsne, rsne_len);
		cp += rsne_len;
	}

	if (rsnxe && rsnxe_len) {
		os_memcpy(cp, rsnxe, rsnxe_len);
		cp += rsnxe_len;
	}

	ori = pos;
	pos = wpa_add_kde(pos, RSN_KEY_DATA_MLO_LINK, buf, cp - buf, NULL, 0);
	wpa_hexdump(MSG_DEBUG, "ML: Link KDE", ori, pos - ori);

	os_free(buf);

	return pos;
}

u8* ml_set_ml_rsno_link_kde(u8 *pos, u8 link_id,
	struct wpa_state_machine *sm, struct wpa_authenticator *wpa_auth)
{
	const u8 *rsnoe, *rsno2e, *rsnxoe;
	size_t rsnoe_len, rsno2e_len, rsnxoe_len;
	size_t kde_len;
	u8 *ori;

	if (!rsn_is_snonce_cookie(sm->SNonce))
		return pos;
	ori = pos;

	rsnoe = get_vendor_ie(wpa_auth->wpa_ie, wpa_auth->wpa_ie_len,
			      RSNE_OVERRIDE_IE_VENDOR_TYPE);
	rsnoe_len = rsnoe ? 2 + rsnoe[1] : 0;

	rsno2e = get_vendor_ie(wpa_auth->wpa_ie, wpa_auth->wpa_ie_len,
			       RSNE_OVERRIDE_2_IE_VENDOR_TYPE);
	rsno2e_len = rsno2e ? 2 + rsno2e[1] : 0;

	rsnxoe = get_vendor_ie(wpa_auth->wpa_ie, wpa_auth->wpa_ie_len,
			       RSNXE_OVERRIDE_IE_VENDOR_TYPE);
	rsnxoe_len = rsnxoe ? 2 + rsnxoe[1] : 0;

	wpa_printf(MSG_DEBUG,
		   "ML: RSN Override Link KDE: link=%u, len=%zu",
		   link_id, RSN_SELECTOR_LEN + rsnoe_len + rsno2e_len +
		   rsnxoe_len);

	/* RSN Override Link KDE */
	*pos++ = WLAN_EID_VENDOR_SPECIFIC;
	kde_len = RSN_SELECTOR_LEN + 1 + rsnoe_len + rsno2e_len +
		rsnxoe_len;
	if (kde_len > 255) {
		wpa_printf(MSG_ERROR,
			   "RSN: RSNOE/RSNO2E/RSNXOE too long (KDE length %zu) to fit in RSN Override Link KDE for link %u",
			   kde_len, link_id);
		return pos;
	}
	*pos++ = kde_len;

	RSN_SELECTOR_PUT(pos, WFA_KEY_DATA_RSN_OVERRIDE_LINK);
	pos += RSN_SELECTOR_LEN;

	*pos++ = link_id;

	if (rsnoe_len) {
		os_memcpy(pos, rsnoe, rsnoe_len);
		pos += rsnoe_len;
	}

	if (rsno2e_len) {
		os_memcpy(pos, rsno2e, rsno2e_len);
		pos += rsno2e_len;
	}

	if (rsnxoe_len) {
		os_memcpy(pos, rsnxoe, rsnxoe_len);
		pos += rsnxoe_len;
	}
	wpa_hexdump(MSG_DEBUG, "ML: RSNO Link KDE", ori, pos - ori);
	return pos;
}

int ml_parse_ie(const u8 *ie, size_t len, struct wpa_ml_ie_parse *ml, u8 *bssid, struct hostapd_data *hapd)
{
	const u8 *pos, *end, *ci_end, *info_end;;
	u16 ml_ctrl;
	size_t ci_len = 0;

	wpa_hexdump(MSG_DEBUG, "ML IE", ie, len);

	os_memset(ml, 0, sizeof(*ml));
	pos = ie + 2; /* skip common ctrl */
	end = ie + len;
	if (pos > end)
		return -1;

	ml_ctrl = WPA_GET_LE16(ie);
	ml->type = ml_ctrl & ML_CTRL_TYPE_MASK;
	if (ml->type != ML_CTRL_TYPE_BASIC) {
		wpa_printf(MSG_INFO, "ML: invalid ML control type = %u",
			ml->type);
		return -1;
	}
	/*common info*/
	/*pso:common info mini len check*/
	if (end - pos < 1) {
		wpa_printf(MSG_ERROR, "ML: No room for common info");
		return -1;
	}
	/*pso: common info len check.*/
	ci_len = *pos;
	if (ci_len > end - pos) {
		wpa_printf(MSG_ERROR, "ML: Truncated Multi-Link Common Info (len=%zu left=%zu)",
			   ci_len, (size_t) (end - pos));
		return -1;
	}
	/*pso: common info header mini len check*/
	if (ci_len < 1 + ETH_ALEN) {
		wpa_printf(MSG_ERROR, "ML: No room for MLD MAC Address in Multi-Link Common Info");
		return -1;
	}
	ci_end = pos + ci_len;

	ml->common_info_len = *pos++;

	wpa_printf(MSG_INFO, "ML: common Info Len = %u", ml->common_info_len);

	/* Check ML control that which common info exist */
	os_memcpy(ml->ml_addr, pos, ETH_ALEN);
	pos += ETH_ALEN;
	wpa_printf(MSG_INFO, "ML: common Info MAC addr = "MACSTR"",
		MAC2STR(ml->ml_addr));

	if (ml_ctrl & ML_CTRL_LINK_ID_INFO_PRESENT) {
		if (ci_end - pos < 1) {
			wpa_printf(MSG_ERROR, "No room for Link ID Info in Multi-Link Common Info");
			return -1;
		}
		ml->link_id = *pos;
		ml->link_id_present = 1;
		wpa_printf(MSG_INFO, "ML: common Info LinkID = %u", ml->link_id);
		pos += 1;
	}
	if (ml_ctrl & ML_CTRL_BSS_PARA_CHANGE_COUNT_PRESENT) {
		if (ci_end - pos < 1) {
			wpa_printf(MSG_ERROR,
				   "No room for BSS Parameters Change Count in Multi-Link Common Info");
			return -1;
		}
		ml->bss_para_change_count = *pos;
		ml->bss_para_change_cnt_present = 1;
		wpa_printf(MSG_INFO, "ML: common Info BssParaChangeCount = %u", *pos);
		pos += 1;
	}
	if (ml_ctrl & ML_CTRL_MEDIUM_SYN_DELAY_INFO_PRESENT) {
		if (ci_end - pos < 2) {
			wpa_printf(MSG_ERROR,
				   "No room for Medium Synchronization Delay Information in Multi-Link Common Info");
			return -1;
		}
		ml->medium_sync_delay = WPA_GET_LE16(pos);
		ml->medium_sync_delay_present = 1;
		wpa_printf(MSG_INFO, "ML: common Info MediumSynDelayInfo = %u", *pos);
		pos += 2;
	}
	if (ml_ctrl & ML_CTRL_EML_CAPA_PRESENT) {
		if (ci_end - pos < 2) {
			wpa_printf(MSG_ERROR,
				   "No room for EML Capabilities in Multi-Link Common Info");
			return -1;
		}
		ml->eml_cap = WPA_GET_LE16(pos);
		ml->eml_cap_present = 1;
		wpa_printf(MSG_INFO, "ML: common Info EML capa = 0x%x", ml->eml_cap);
		pos += 2;
	}
	if (ml_ctrl & ML_CTRL_MLD_CAPA_PRESENT) {
		if (ci_end - pos < 2) {
			wpa_printf(MSG_ERROR,
				   "No room for MLD Capabilities and Operations in Multi-Link Common Info");
			return -1;
		}
		ml->mld_cap = WPA_GET_LE16(pos);
		ml->mld_cap_present = 1;
		wpa_printf(MSG_INFO, "ML: common Info MLD capa = 0x%x", ml->mld_cap);
		pos += 2;
	}
	if (ml_ctrl & ML_CTRL_MLD_ID_PRESENT) {
		if (ci_end - pos < 1) {
			wpa_printf(MSG_ERROR,
				   "No room for AP MLD ID in Multi-Link Common Info");
			return -1;
		}
		ml->mld_id = *pos;
		ml->mld_id_present = 1;
		wpa_printf(MSG_INFO, "ML:common Info MLD id = %u", ml->mld_id);
		pos += 1;
	}
	if (ml_ctrl & ML_CTRL_EXT_MLD_CAP_PRESENT) {
		if (ci_end - pos < 2) {
			wpa_printf(MSG_ERROR,
					"No room for Extended MLD Cap and Operations of Commmon Info");
			return -1;
		}
		pos += 2;
	}

	if (pos - (ie + 2) != ml->common_info_len) {
		ml->valid = false;
		wpa_printf(MSG_INFO, "ML: invalid ML control info len = %u",
			ml->common_info_len);
		return -1;
	} else {
		ml->valid = true;
	}

	/* pos point to link info, recusive parse it */
	while (pos < end) {
		u16 sta_ctrl;
		struct per_sta_profile *profile;
		u8 sta_info_len;
		const u8 *head, *tail;
		u8 frag_flag = 0;

		if (*pos != ML_SUB_ID_PER_STA_PROFILE ||
			ml->prof_num >= ML_MAX_LINK_NUM)
			break;

		head = pos + 2;
		tail = head + pos[1];
		/*pso: subtvl mini length check.*/
		if (head > tail) {
			wpa_printf(MSG_ERROR, "ML: Truncated Per-STA Profile subelement");
			continue;
		}

		if (pos[1] == 255)
			frag_flag = 1;
		pos += 2;
		sta_ctrl = WPA_GET_LE16(pos);
		pos += 2;
		/*pso: left data len check*/
		if (tail - pos < 1) {
			wpa_printf(MSG_ERROR, "ML: No room for STA Info field");
			continue;
		}
		/*pso: sta info len check */
		ci_len = *pos;
		if (ci_len < 1 || ci_len > tail - pos) {
			wpa_printf(MSG_INFO, "ML: faild Truncated STA Info field(ci_len: %zu, tail - pos: %zu)",
				ci_len, tail - pos);
			continue;
		}
		info_end = pos + ci_len;

		profile = &ml->profiles[ml->prof_num++];
		profile->link_id = sta_ctrl & ML_STA_CTRL_LINK_ID_MASK;
		profile->complete_profile =
			(sta_ctrl & ML_STA_CTRL_COMPLETE_PROFILE) > 0;

		wpa_printf(MSG_INFO, "ML: LinkID=%u Ctrl=0x%x(%s) Total=%u",
			profile->link_id, sta_ctrl,
			profile->complete_profile ? "COMPLETE" : "PARTIAL",
			ml->prof_num);

		sta_info_len = *pos++;

		if (sta_ctrl & ML_STA_CTRL_MAC_ADDR_PRESENT) {
			if (info_end - pos < ETH_ALEN) {
				wpa_printf(MSG_ERROR,
					   "ML: Truncated STA MAC Address in STA Info");
				continue;
			}
			os_memcpy(profile->addr, pos, ETH_ALEN);
			profile->mac_addr_present = 1;
			wpa_printf(MSG_INFO, "ML: LinkID=%u, LinkAddr="MACSTR"",
				profile->link_id, MAC2STR(profile->addr));
			if (hapd) {
				struct hostapd_iface *iface = hapd->iface;
				if (iface) {
					struct hostapd_iface *h = iface->interfaces->iface[ml->prof_num];
					struct hostapd_data *h_hapd = h->bss[0];
					struct sta_info *sta = NULL;
					sta = ap_get_sta(h_hapd, profile->addr);
					if (sta)
						continue;
					sta = ap_sta_add(h_hapd, profile->addr);
					if (sta) {
						sta->flags |= WLAN_STA_AUTH | WLAN_STA_ASSOC;
						os_memset(hapd->profiles[ml->prof_num].addr, 0, ETH_ALEN);
						os_memcpy(hapd->profiles[ml->prof_num].addr, profile->addr, ETH_ALEN);
					}
				}
			}
			pos += ETH_ALEN;
		}
		if (sta_ctrl & ML_STA_CTRL_BCN_INTV_PRESENT) {
			if (info_end - pos < 2) {
				wpa_printf(MSG_INFO,
					   "ML:Truncated Beacon Interval in STA Info");
				continue;
			}
			profile->beacon_interval = WPA_GET_LE16(pos);
			profile->bcn_intvl_present = 1;
			wpa_printf(MSG_INFO, "ML: LinkID=%u, BCN_INTV = %u",
				profile->link_id, profile->beacon_interval);
			pos += 2;
		}
		if (sta_ctrl & ML_STA_CTRL_TSF_OFFSET_PRESENT) {
			if (info_end - pos < 8) {
				wpa_printf(MSG_INFO,
					   "ML: Truncated TSF Offset in STA Info");
				continue;
			}
			os_memcpy(&profile->tsf_offset, pos, 8);
			profile->tsf_offset_present = 1;
			wpa_printf(MSG_INFO, "ML: LinkID=%u, TSF_OFFSET = %"PRIu64,
				profile->link_id, profile->tsf_offset);
			pos += 8;
		}
		if (sta_ctrl & ML_STA_CTRL_DTIM_INFO_PRESENT) {
			if (info_end - pos < 2) {
				wpa_printf(MSG_INFO,
					   "ML:Truncated TSF Offset in STA Info");
				continue;
			}
			profile->dtim = WPA_GET_LE16(pos);
			profile->dtim_present = 1;
			wpa_printf(MSG_INFO, "ML: LinkID=%u, DTIM_INFO = 0x%x",
				profile->link_id, profile->dtim);
			pos += 2;
		}
		/* If the Complete Profile subfield = 1 and
		 * NSTR Link Pair Present = 1, then NSTR Indication Bitmap exist
		 * NSTR Bitmap Size = 1 if the length of the corresponding
		 * NSTR Indication Bitmap is 2 bytes, and = 0 if the
		 * length of the corresponding NSTR Indication Bitmap = 1 byte
		 */
		if ((sta_ctrl & ML_STA_CTRL_COMPLETE_PROFILE) &&
			(sta_ctrl & ML_STA_CTRL_NSTR_LINK_PAIR_PRESENT)) {
			if (((sta_ctrl & ML_STA_CTRL_NSTR_BMP_SIZE) >>
				ML_STA_CTRL_NSTR_BMP_SIZE_SHIFT) == 0) {
				if (info_end - pos < 1) {
					wpa_printf(MSG_INFO,
						   "ML: Truncated NSTR Indication Bitmap in STA Info");
					continue;
				}
				profile->nstr_bmap = *pos;
				wpa_printf(MSG_INFO, "ML: LinkID=%u, NSTR_BMP0=0x%x",
					profile->link_id, profile->nstr_bmap);
				pos += 1;
			} else {
				if (info_end - pos < 2) {
					wpa_printf(MSG_INFO,
						   "ML:Truncated NSTR Indication Bitmap in STA Info");
					continue;
				}
				profile->nstr_bmap = WPA_GET_LE16(pos);
				wpa_printf(MSG_INFO, "ML: LinkID=%u, NSTR_BMP1=0x%x",
					profile->link_id, profile->nstr_bmap);
				pos += 2;
			}
			profile->nstr_present = 1;
		}
		if (sta_ctrl & ML_STA_CTRL_BSS_CHG_CNT_PRESENT) {
			if (info_end - pos < 1) {
				wpa_printf(MSG_INFO,
					   "ML:Truncated BSS Parameters Change Count in STA Info");
				continue;
			}
			profile->bss_para_change_count = *pos;
			profile->bss_para_change_count_present = 1;
			wpa_printf(MSG_INFO, "ML: LinkID=%u, BSS_CHG_CNT = %u",
				profile->link_id, profile->bss_para_change_count);
			pos += 1;
		}
		if (pos - (head + 2) != sta_info_len) {
			wpa_printf(MSG_WARNING, "ML: invalid ML STA info len = %u",
				sta_info_len);
			ml->prof_num--;
		}

		/* point to next Per-STA profile*/
		pos = tail;

		/* process sta profile subelement fragments, skip other information
		 * because we do not need them.
		 */
		while (frag_flag && (pos + 1) < end) {
			u8 frag_len = 0;
			if (*pos != 254) {
				wpa_printf(MSG_INFO,
					"ML: invalid sta profile FragID(254) %u", *pos);
				break;
			}
			frag_len = *(pos + 1);
			frag_flag = frag_len == 255 ? 1 : 0;
			pos += frag_len + 2;
		}
	}

	return 0;
}


#ifdef CONFIG_SAE
int ml_sae_process_auth(struct sae_data *sae, u16 auth_transaction,
	const u8 *ies, size_t ies_len)
{
	struct ieee802_11_elems elems;
	struct wpa_ml_ie_parse ml;
	struct wpabuf *frag_ml_ie = NULL;
	const u8 *ml_ie;
	size_t ml_ie_len = 0;

	if (!sae)
		return -1;

	wpa_printf(MSG_DEBUG, "ML: SAE Possible elements at the end of the frame");
	wpa_hexdump(MSG_DEBUG, "ML: SAE Possible elements at the end of the frame",
			    ies, ies_len);

	if (ieee802_11_parse_elems(ies, ies_len, &elems, 1) == ParseFailed) {
		wpa_printf(MSG_DEBUG, "ML: SAE failed to parse elements");
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

	if (auth_transaction == 1) {
		os_free(sae->peer_ml_ie);
		if (!ml_ie) {
			wpa_printf(MSG_DEBUG, "ML: SAE clearing STA ML IE");
			sae->peer_ml_ie = NULL;
			sae->dot11MultiLinkActivated = 0;
		} else {
			if (ml_parse_ie(ml_ie, ml_ie_len, &ml, NULL, NULL) != 0) {
				sae->peer_ml_ie = NULL;
				sae->dot11MultiLinkActivated = 0;
				if (frag_ml_ie)
					wpabuf_free(frag_ml_ie);
				return -1;
			} else {
				sae->peer_ml_ie = os_memdup(&ml, sizeof(ml));
				if (sae->peer_ml_ie == NULL) {
					sae->dot11MultiLinkActivated = 0;
					if (frag_ml_ie)
						wpabuf_free(frag_ml_ie);
					return -1;
				}
				os_memcpy(sae->peer_ml_addr, ml.ml_addr, ETH_ALEN);
				sae->dot11MultiLinkActivated = 1;
			}
		}
	} else if (auth_transaction == 2) {
		if (sae->dot11MultiLinkActivated && !ml_ie) {
			wpa_printf(MSG_ERROR, "ML: SAE confirm should have ml ie");
			if (frag_ml_ie)
				wpabuf_free(frag_ml_ie);
			return -1;
		} else if (!sae->dot11MultiLinkActivated && ml_ie) {
			wpa_printf(MSG_ERROR, "ML: SAE confirm should not have ml ie");
			if (frag_ml_ie)
				wpabuf_free(frag_ml_ie);
			return -1;
		}

		if (ml_ie) {
			if (ml_parse_ie(ml_ie, ml_ie_len, &ml, NULL, NULL) != 0) {
				wpa_printf(MSG_ERROR, "ML: SAE confirm failed to parse ml ie");
				if (frag_ml_ie)
					wpabuf_free(frag_ml_ie);
				return -1;
			} else if (os_memcmp(sae->peer_ml_ie->ml_addr, ml.ml_addr, ETH_ALEN) != 0) {
				wpa_printf(MSG_DEBUG,
					"ML: SAE trans = %u, mismatch ML addr (peer="MACSTR", recv="MACSTR")",
					auth_transaction, MAC2STR(sae->peer_ml_ie->ml_addr), MAC2STR(ml.ml_addr));
				if (frag_ml_ie)
					wpabuf_free(frag_ml_ie);
				return -1;
			}
		}
	} else {
		wpa_printf(MSG_DEBUG,
		       "ML: unexpected SAE authentication transaction %u",
		       auth_transaction);
		if (frag_ml_ie)
			wpabuf_free(frag_ml_ie);
		return -1;
	}

	wpa_printf(MSG_DEBUG, "(%s)[%d]ML: dot11MultiLinkActivated(%u). own_ml_addr"MACSTR", peer_ml_addr"MACSTR"",
		__func__, __LINE__, sae->dot11MultiLinkActivated, MAC2STR(sae->own_ml_addr),MAC2STR(sae->peer_ml_addr));
	if (frag_ml_ie)
		wpabuf_free(frag_ml_ie);
	return 0;
}

int ml_sae_write_auth(struct hostapd_data *hapd,
		      struct sae_data *sae, struct wpabuf *buf)
{
	u16 ctrl = 0;
	size_t i;

	if (!sae || !sae->dot11MultiLinkActivated || !hapd || !hapd->ml_group)
		return 0;

	wpa_printf(MSG_DEBUG, "ML: write ml ie for sae auth");

	wpabuf_put_u8(buf, WLAN_EID_EXTENSION);
	/* extid: 1, common ctrl: 2, common info: 7(len:1, mac:6) */
	wpabuf_put_u8(buf, 10);
	wpabuf_put_u8(buf, WLAN_EID_EXT_MLD);

	/* ml common control */
	ML_SET_CTRL_TYPE(ctrl, ML_CTRL_TYPE_BASIC);

	/* A Basic Multi-Link element in an Authentication frame:
	 * the STA shall include the MLD MAC address of the MLD
	 * the STA shall set all subfields in the Presence Bitmap subfield of
	 * the Multi-Link Control field of the element to 0
	 * the STA shall not include the Link Info field of the element.
	 */
	ML_SET_CTRL_PRESENCE(ctrl, 0);
	wpabuf_put_le16(buf, ctrl);

	/* len:1, mac:6 */
	wpabuf_put_u8(buf, 7);

	/* ml mac addr */
	wpabuf_put_data(buf, hapd->ml_group->ml_addr, ETH_ALEN);

	return 0;
}
#endif /* CONFIG_SAE */



/* AP */
const u8 * ml_auth_spa(struct wpa_state_machine *sm, const u8 *peer_addr)
{
	if(sm && peer_addr && sm->dot11MultiLinkActivated) {
		if (os_memcmp(peer_addr, sm->sta_ml_ie->ml_addr, ETH_ALEN) != 0) {
			wpa_printf(MSG_INFO,
				"ML: SPA[" MACSTR "] use ml addr[" MACSTR "]",
				MAC2STR(peer_addr), MAC2STR(sm->sta_ml_ie->ml_addr));
			return sm->sta_ml_ie->ml_addr;
		}
	}

	return peer_addr;
}

const u8 * ml_auth_aa(struct wpa_state_machine *sm, const u8 *addr)
{
	if(sm && addr && sm->dot11MultiLinkActivated) {
		struct wpa_ml_group *ml_group = STATE_MACHINE_ML_GROUP;

		if (ml_group && os_memcmp(ml_group->ml_addr, addr, ETH_ALEN) != 0) {
			wpa_printf(MSG_INFO,
				"ML: AA[" MACSTR "] use ml addr[" MACSTR "]",
				MAC2STR(addr), MAC2STR(ml_group->ml_addr));
			return ml_group->ml_addr;
		}
	}

	return addr;
}

/* AP */
struct wpa_ml_link * ml_setup_link(struct hostapd_data *hapd,
	struct wpa_ml_group *ml_group, u8 link_id, u8 *mld_addr)
{
	struct wpa_ml_link *links, *link;

	links = os_realloc_array(ml_group->links, ml_group->ml_link_num + 1,
				 sizeof(struct wpa_ml_link));
	if (links == NULL || hapd == NULL) {
		wpa_printf(MSG_ERROR, "ML: links alloc fail");
		return NULL;
	}
	ml_group->links = links;
	link = &links[ml_group->ml_link_num++];
	link->ctx = hapd;
	link->link_id = link_id;
	os_memcpy(link->addr, hapd->own_addr, ETH_ALEN);
	os_memcpy(ml_group->ml_addr, mld_addr, ETH_ALEN);  /*for mlo reconfig*/
	hapd->ml_group = ml_group;

	wpa_printf(MSG_INFO, "ML: Join ML Group=%p, link:%p, link_id=%u, ml_group->ml_link_num=%zu", ml_group, link, link_id, ml_group->ml_link_num);

	return link;
}

struct wpa_ml_group *ml_alloc_group(struct hostapd_data *hapd,
				    u8 group_id, u8 *mld_addr)
{
	struct wpa_ml_group *ml_group = NULL;
	struct wpa_ml_link *link;
	u8 i;

	ml_group = os_zalloc(sizeof(*ml_group));
	if (ml_group == NULL) {
		wpa_printf(MSG_ERROR, "ML: ml_group alloc fail");
		return NULL;
	}

	ml_group->ctx = hapd;
	os_memcpy(ml_group->ml_addr, mld_addr, ETH_ALEN);
	ml_group->ml_group_id = group_id;

	wpa_printf(MSG_INFO,
		"ML: Alloc ML Group=%p (ml_group_id=%u, ml_addr=" MACSTR ")",
		ml_group, group_id, MAC2STR(ml_group->ml_addr));

	return ml_group;
}

struct wpa_ml_group * ml_get_group(struct hapd_interfaces *interfaces,
				u8 group_id)
{
	size_t i, j;

	/* search interfaces to find existed ml group */
	for (i = 0; i < interfaces->count; i++) {
		struct hostapd_iface *iface = interfaces->iface[i];

		for (j = 0; j < iface->num_bss; j++) {
			struct hostapd_data *hapd = iface->bss[j];

			if (hapd->ml_group &&
			    hapd->ml_group->ml_group_id == group_id) {
				return hapd->ml_group;
			}
		}
	}

	return NULL;
}

int ml_group_init(struct hostapd_data *hapd,
		u8 mld_grp_idx, u8 link_id, u8 *mld_addr)
{
	struct hapd_interfaces *interfaces = hapd->iface->interfaces;
	struct wpa_ml_group *ml_group = NULL;
	struct wpa_ml_link *link;
	u8 i;

	wpa_printf(MSG_INFO, "ML: " MACSTR " ml_group_init, ml_group_id=%u",
			MAC2STR(hapd->own_addr), mld_grp_idx);

	if (!interfaces)
		goto done;

	ml_group = ml_get_group(interfaces, mld_grp_idx);

	/* found, join it */
	if (ml_group) {
		wpa_printf(MSG_INFO,"old mld addr " MACSTR ", new mld addr " MACSTR " ", MAC2STR(ml_group->ml_addr),MAC2STR(mld_addr));
		if (os_memcmp(ml_group->ml_addr, mld_addr, ETH_ALEN) != 0) {
			wpa_printf(MSG_INFO,"mld addr changed copy new addr set");
			os_memcpy(ml_group->ml_addr, mld_addr, ETH_ALEN);
		}
		/* error check */
		for (i = 0; i < ml_group->ml_link_num; i++) {
			link = &ml_group->links[i];
			if (link->ctx == hapd) {
				wpa_printf(MSG_INFO, "ML: reinit link:%p", link);
				return -1;
			}
		}
		wpa_printf(MSG_INFO, "ML: group has created,join it");
		if (ml_setup_link(hapd, ml_group, link_id, mld_addr) == NULL)
			return -1;
	} else {
		ml_group = ml_alloc_group(hapd, mld_grp_idx, mld_addr);
		if (ml_group == NULL)
			return -1;
		wpa_printf(MSG_INFO, "ML: creat group");
		if (ml_setup_link(hapd, ml_group, link_id, mld_addr) == NULL) {
			os_free(ml_group);
			return -1;
		}
	}

done:
	return 0;
}

int ml_group_deinit_for_reconfig(struct hostapd_data *hapd)
{
	struct wpa_ml_group *ml_group = hapd->ml_group;
	struct wpa_ml_link *link;
	size_t i, k = 0;

	if (!ml_group)
		return -1;

	for (i = 0; i < ml_group->ml_link_num; i++) {
		link = &ml_group->links[i];
		if (link == NULL)
			continue;

		if (link->ctx == hapd) {
			wpa_printf(MSG_INFO, "ML: mlo reconfig Remove link %u", link->link_id);
			k = i;
			while (k < (ml_group->ml_link_num - 1)) {
				os_memcpy(&ml_group->links[k],
					&ml_group->links[k + 1], sizeof(*link));
				k++;
			}
			ml_group->ml_link_num--;
			if (ml_group->ml_link_num >=0 && ml_group->ml_link_num <16) {
				wpa_printf(MSG_INFO, "ML: mlo reconfig free the link num:%zu, links:%p", ml_group->ml_link_num + 1, &ml_group->links[ml_group->ml_link_num]);
				ml_group->links = os_realloc_array(ml_group->links, ml_group->ml_link_num, sizeof(struct wpa_ml_link));
			}
		}
	}

	hapd->ml_group = NULL;
	if (ml_group->ml_link_num == 0 && ml_group->links) {
		wpa_printf(MSG_INFO, "(%s)[%d] ML: reconfig ml_group=%p, will free. ml_group->ctx(%p), hapd(%p)",
			__func__, __LINE__, ml_group, ml_group->ctx, hapd);
		os_free(ml_group->links);
		ml_group->links = NULL;
		os_free(ml_group);
	}

	return 0;
}

int ml_group_deinit(struct hostapd_data *hapd)
{
	struct wpa_ml_group *ml_group = hapd->ml_group;
	struct wpa_ml_link *link;
	size_t i, k = 0;

	if (!ml_group)
		return -1;

	/* find the link, del it*/
	for (i = 0; i < ml_group->ml_link_num; i++) {
		link = &ml_group->links[i];
		if (link == NULL)
			continue;

		if (link->ctx == hapd) {
			wpa_printf(MSG_INFO, "ML: Remove link %d,iface(%s) mac addr("MACSTR")",
				link->link_id, hapd->conf->iface, MAC2STR(link->addr));
			k = i;
			while (ml_group->ml_link_num >= 1 && (k < (ml_group->ml_link_num - 1)) ) {
				os_memcpy(&ml_group->links[k],
					&ml_group->links[k + 1], sizeof(*link));
				k++;
			}
			os_memset(&ml_group->links[k], 0, sizeof(*link));
			ml_group->ml_link_num--;
		}
	}

	hapd->ml_group = NULL;
	if (ml_group->ml_link_num == 0 && ml_group->links) {
		wpa_printf(MSG_INFO, "(%s)[%d] ML: ml_group=%p, will free. ml_group->ctx(%p), hapd(%p)",
			__func__, __LINE__, ml_group, ml_group->ctx, hapd);
		os_free(ml_group->links);
		ml_group->links = NULL;
		os_free(ml_group);
	}
	return 0;
}


u8 ml_get_link_id(struct wpa_state_machine *sm)
{
	struct wpa_ml_group *ml_group = STATE_MACHINE_ML_GROUP;
	struct hostapd_data *hapd = (struct hostapd_data *)sm->wpa_auth->cb_ctx;
	struct wpa_ml_link *link;
	size_t i;

	for (i = 0; i < ml_group->ml_link_num; i++) {
		link = &ml_group->links[i];

		if (link->ctx == hapd)
			return link->link_id;
	}

	return 0xff;
}



/* Volatile address metadata for the shared wireless sampler. This is not
 * association membership, a station query, a key cache, or a diagnostic log. */
#define LG6851F_IDENTITY_DIR "/tmp/lg6851f-wifi-identities"

static int ml_identity_path(struct wpa_state_machine *sm, char *path, size_t len)
{
    struct hostapd_data *hapd;
    if (!sm || !sm->wpa_auth || !sm->wpa_auth->cb_ctx)
        return -1;
    hapd = sm->wpa_auth->cb_ctx;
    return os_snprintf(path, len, LG6851F_IDENTITY_DIR "/"
                       MACSTR "-" MACSTR ".json",
                       MAC2STR(hapd->own_addr), MAC2STR(sm->addr));
}

void ml_identity_remove(struct wpa_state_machine *sm)
{
    char path[160];
    int n = ml_identity_path(sm, path, sizeof(path));
    if (n > 0 && n < (int) sizeof(path))
        unlink(path);
}

static void ml_identity_publish(struct wpa_state_machine *sm)
{
    char path[160], temporary[164];
    FILE *f;
    size_t i;
    int n, failed;
    struct wpa_ml_ie_parse *ml;

    if (!sm || !sm->dot11MultiLinkActivated || !(ml = sm->sta_ml_ie) ||
        !ml->valid || ml->prof_num > ML_MAX_LINK_NUM ||
        is_zero_ether_addr(ml->ml_addr) || is_multicast_ether_addr(ml->ml_addr))
        return;
    n = ml_identity_path(sm, path, sizeof(path));
    if (n <= 0 || n >= (int) sizeof(path))
        return;
    if (mkdir(LG6851F_IDENTITY_DIR, 0700) && errno != EEXIST)
        return;
    os_snprintf(temporary, sizeof(temporary), "%s.new", path);
    f = fopen(temporary, "w");
    if (!f)
        return;
    /* Only addresses and process ownership are exported. Never export IEs,
     * passwords, SAE material, PMKs, or key data. Atomic replace avoids tears. */
    fprintf(f, "{\"pid\":%ld,\"mld_mac\":\"" MACSTR "\",\"links\":[\""
            MACSTR "\"", (long) getpid(), MAC2STR(ml->ml_addr), MAC2STR(sm->addr));
    for (i = 0; i < ml->prof_num; i++) {
        const u8 *addr = ml->profiles[i].addr;
        if (!ml->profiles[i].mac_addr_present || is_zero_ether_addr(addr) ||
            is_multicast_ether_addr(addr))
            continue;
        fprintf(f, ",\"" MACSTR "\"", MAC2STR(addr));
    }
    fputs("]}\n", f);
    failed = ferror(f);
    if (fclose(f))
        failed = 1;
    if (failed || rename(temporary, path))
        unlink(temporary);
}

int ml_new_assoc_sta(struct wpa_state_machine *sm, const u8 *ie, size_t len)
{
	if (!sm)
		return -1;
	struct hostapd_data *hapd = (struct hostapd_data *)sm->wpa_auth->cb_ctx;
	if (!hapd)
		return -1;

	wpa_printf(MSG_INFO, "ML: new STA:");
	ml_identity_remove(sm);
	os_free(sm->sta_ml_ie);
	if (ie == NULL || len == 0 || STATE_MACHINE_ML_GROUP == NULL) {
		sm->sta_ml_ie = NULL;
		sm->dot11MultiLinkActivated = 0;
	} else {
		struct wpa_ml_ie_parse ml;
		struct wpa_ml_group *ml_group = STATE_MACHINE_ML_GROUP;

		if (ml_parse_ie(ie, len, &ml, NULL, hapd) != 0) {
			sm->sta_ml_ie = NULL;
			sm->dot11MultiLinkActivated = 0;
			return -1;
		} else {
			sm->sta_ml_ie = os_memdup(&ml, sizeof(ml));
			if (sm->sta_ml_ie == NULL) {
				sm->dot11MultiLinkActivated = 0;
				return -1;
			}

			sm->sta_ml_ie->link_id = ml_get_link_id(sm);
			sm->dot11MultiLinkActivated = 1;
			ml_identity_publish(sm);
			wpa_printf(MSG_INFO, "ML: new STA:dot11MultiLinkActivated(%u),(ml_addr:" MACSTR ")",
				sm->dot11MultiLinkActivated, MAC2STR(sm->sta_ml_ie->ml_addr));
		}
	}


	return 0;
}

u8* ml_add_m1_kde(struct wpa_state_machine *sm, u8 *pos)
{
	if (!sm->dot11MultiLinkActivated)
		return pos;

	wpa_printf(MSG_INFO, "ML: Add Mac:(" MACSTR ") into EAPOL-Key 1/4", MAC2STR(STATE_MACHINE_ML_GROUP_ADDR));
	return ml_set_mac_kde(pos, STATE_MACHINE_ML_GROUP_ADDR);
}

int ml_process_m2_kde(struct wpa_state_machine *sm,
			const u8 *key_data, size_t key_data_len)
{
	struct wpa_eapol_ie_parse kde;
	size_t i, j;

	if (wpa_parse_kde_ies(key_data, key_data_len, &kde) != 0 ||
		!sm->dot11MultiLinkActivated)
		return 0;

	if (!kde.mac_addr) {
		wpa_printf(MSG_INFO, "ML: EAPOL-Key 2/4 no ml addr");
		return -1;
	}

	if (os_memcmp(sm->sta_ml_ie->ml_addr, kde.mac_addr, ETH_ALEN) != 0) {
		wpa_printf(MSG_INFO,
		"ML: EAPOL-Key 2/4 wrong ml addr ["MACSTR"] expect ["MACSTR"]",
			MAC2STR(kde.mac_addr), MAC2STR(sm->sta_ml_ie->ml_addr));
		return -1;
	}

	/* single link doesn't need profile and mlo link kde */
	if (sm->sta_ml_ie->prof_num != kde.mlo_link.num &&
		sm->sta_ml_ie->prof_num + 1 != kde.mlo_link.num) {
		wpa_printf(MSG_INFO,
			"ML: EAPOL-Key 2/4 mlo link num mismatch (kde=%d, prof=%u)",
			(int)kde.mlo_link.num,
			sm->sta_ml_ie->prof_num);
		return -2;
	}

	wpa_printf(MSG_INFO,
		"ML: EAPOL-Key 2/4 mlo setup link ["MACSTR", link_id=%u]",
		MAC2STR(sm->addr), sm->sta_ml_ie->link_id);

	for (i = 0; i < kde.mlo_link.num; i++) {
		struct wpa_mlo_link_kde *mlo_link =
			(struct wpa_mlo_link_kde *) kde.mlo_link.kdes[i].data;

		wpa_printf(MSG_INFO,
			"ML: EAPOL-Key 2/4 mlo kde link ["MACSTR", link_id=%u]",
			MAC2STR(mlo_link->addr), mlo_link->info & 0xf);

		if (kde.mlo_link.kdes[i].len < 7) {
			wpa_printf(MSG_INFO,
				"ML: EAPOL-Key 2/4 error mlo link len=%d",
				(int)kde.mlo_link.kdes[i].len);
			return -3;
		}

		if (os_memcmp(sm->addr, mlo_link->addr, ETH_ALEN) == 0 &&
				sm->sta_ml_ie->link_id == (mlo_link->info & 0xf))
			continue;

		for (j = 0; j < sm->sta_ml_ie->prof_num; j++) {
			if (os_memcmp(sm->sta_ml_ie->profiles[j].addr,
					mlo_link->addr, ETH_ALEN) == 0 &&
				sm->sta_ml_ie->profiles[j].link_id ==
					(mlo_link->info & 0xf))
				break;
		}

		if (j == sm->sta_ml_ie->prof_num) {
			wpa_printf(MSG_INFO,
				"ML: EAPOL-Key 2/4 mlo link ["MACSTR", link_id=%u] not matched",
				MAC2STR(mlo_link->addr), mlo_link->info & 0xf);
			return -4;
		}
	}

	return 0;
}

u8* ml_add_m3_kde(struct wpa_state_machine *sm, u8 *pos)
{
	struct wpa_ml_group *ml_group = NULL;
	struct wpa_ml_link *link;
	u8 i,j;

	if (!sm->dot11MultiLinkActivated)
		return pos;

	wpa_printf(MSG_INFO, "ML: Add Mac/Link/GTK into EAPOL-Key 3/4");
	ml_group = STATE_MACHINE_ML_GROUP;
	pos = ml_set_mac_kde(pos, ml_group->ml_addr);

	for (i = 0; i < ml_group->ml_link_num; i++) {
		struct wpa_authenticator *auth;
		u8 found = false;
#ifdef CONFIG_IEEE80211R_AP
		u8 *rsn_ie_buf = NULL;
		const u8 *mde;
		size_t mde_len;
#endif
		const u8 *rsne, *rsnxe;
		size_t rsne_len = 0, rsnxe_len = 0;

		link = &ml_group->links[i];
		if (link->link_id == sm->sta_ml_ie->link_id) {
			found = true;
		} else {
			for (j = 0; j < sm->sta_ml_ie->prof_num; j++) {
				if (sm->sta_ml_ie->profiles[j].link_id ==
					link->link_id)
					found = true;
			}
		}
		if (!found)
			continue;

		auth = ((struct hostapd_data *)link->ctx)->wpa_auth;
		if (!auth) {
			wpa_printf(MSG_ERROR,
					"ML: wpa_auth is NULL--link_id=%u, link_addr=" MACSTR "",
					link->link_id, MAC2STR(link->addr));
			return NULL;
		}

		rsne = get_ie(auth->wpa_ie, auth->wpa_ie_len, WLAN_EID_RSN);
		rsne_len = rsne ? rsne[1] + 2 : 0;
		rsnxe = get_ie(auth->wpa_ie, auth->wpa_ie_len, WLAN_EID_RSNX);
		rsnxe_len = rsnxe ? rsnxe[1] + 2 : 0;

#ifdef CONFIG_IEEE80211R_AP
		if (wpa_key_mgmt_ft(sm->wpa_key_mgmt) && rsne) {
			int res;
			wpa_hexdump(MSG_MSGDUMP, "ML: WPA IE before FT processing",
					rsne, rsne_len);

			mde = get_ie(auth->wpa_ie, auth->wpa_ie_len, WLAN_EID_MOBILITY_DOMAIN);
			mde_len = mde ? mde[1] + 2 : 0;

			wpa_hexdump(MSG_MSGDUMP, "ML: MDE", mde, mde_len);

			if (mde && i == 0) {
				os_memcpy(pos, mde, mde_len);
				pos += mde_len;
			}

			/* Add PMKR1Name into RSN IE (PMKID-List) */
			rsn_ie_buf = os_malloc(rsne_len + 2 + 2 + PMKID_LEN);
			if (rsn_ie_buf == NULL) {
				wpa_printf(MSG_INFO, "ML: OOM for FT");
				return pos;
			}
			os_memcpy(rsn_ie_buf, rsne, rsne_len);
			res = wpa_insert_pmkid(rsn_ie_buf, &rsne_len,
						   sm->pmk_r1_name);
			if (res < 0) {
				wpa_printf(MSG_INFO, "ML: insert pmk for FT failed");
				os_free(rsn_ie_buf);
				return pos;
			}
			wpa_hexdump(MSG_MSGDUMP,
					"ML: WPA IE after PMKID[PMKR1Name] addition into RSNE",
					rsn_ie_buf, rsne_len);
			rsne = rsn_ie_buf;
		}
#endif /* CONFIG_IEEE80211R_AP */

		pos = ml_set_ml_link_kde(pos, link->link_id, link->addr,
			rsne, rsne_len, rsnxe, rsnxe_len);
		pos = ml_set_ml_rsno_link_kde(pos, link->link_id, sm, auth);
		pos = ml_set_gtk_kde(sm, pos, link);
		pos = ml_set_ieee80211w_kde(sm, pos, link);

#ifdef CONFIG_IEEE80211R_AP
		os_free(rsn_ie_buf);
#endif
	}

	return pos;
}

int ml_process_m4_kde(struct wpa_state_machine *sm,
		const u8 *key_data, size_t key_data_len)
{
	struct wpa_eapol_ie_parse kde;

	if (wpa_parse_kde_ies(key_data, key_data_len, &kde) != 0 ||
	    !sm->dot11MultiLinkActivated)
		return 0;

 if (!kde.mac_addr) {
		wpa_printf(MSG_INFO, "ML: EAPOL-Key 4/4 no ml addr");
		return -1;
	}

	if (os_memcmp(sm->sta_ml_ie->ml_addr, kde.mac_addr, ETH_ALEN) != 0) {
		wpa_printf(MSG_INFO, "ML: EAPOL-Key 4/4 wrong ml addr");
		return -1;
	}

	return 0;
}

u8* ml_set_gtk_kde(struct wpa_state_machine *sm, u8 *pos,
		   struct wpa_ml_link *link)
{
	struct wpa_authenticator *auth =
		((struct hostapd_data *)link->ctx)->wpa_auth;
	struct wpa_group *gsm = auth->group;
	int gtkidx;
	u8 *gtk, dummy_gtk[32], *ori;
	size_t gtk_len;
	struct wpa_auth_config *conf = &sm->wpa_auth->conf;
	u8 hdr[7];

	if (sm->wpa != WPA_VERSION_WPA2)
		return pos;

	gtk = gsm->GTK[gsm->GN - 1];
	gtk_len = gsm->GTK_len;
	if (conf->disable_gtk ||
	    sm->wpa_key_mgmt == WPA_KEY_MGMT_OSEN) {
		/*
		 * Provide unique random GTK to each STA to prevent use
		 * of GTK in the BSS.
		 */
		if (random_get_bytes(dummy_gtk, gtk_len) < 0)
			goto done;
		gtk = dummy_gtk;
	}
	gtkidx = gsm->GN;

	os_memset(hdr, 0, 7);
	hdr[0] = (gtkidx & 0x03) | (link->link_id & 0x0f) << 4;
	ori = pos;
	pos = wpa_add_kde(pos, RSN_KEY_DATA_MLO_GTK, hdr, 7,
			  gtk, gtk_len);
	wpa_hexdump(MSG_DEBUG, "ML: GTK KDE", ori, pos - ori);
done:
	return pos;
}

static inline int ml_get_seqnum(struct wpa_authenticator *wpa_auth,
				      const u8 *addr, int idx, u8 *seq)
{
	int res;

	if (!wpa_auth->cb->get_seqnum)
		return -1;
	res = wpa_auth->cb->get_seqnum(wpa_auth->cb_ctx, addr, idx, seq);
	return res;
}

u8* ml_set_ieee80211w_kde(struct wpa_state_machine *sm, u8 *pos,
			  struct wpa_ml_link *link)
{
	struct wpa_authenticator *auth =
		((struct hostapd_data *)link->ctx)->wpa_auth;
	struct wpa_mlo_igtk_kde igtk;
	struct wpa_mlo_bigtk_kde bigtk;
	struct wpa_group *gsm = auth->group;
	u8 rsc[WPA_KEY_RSC_LEN], *ori;
	struct wpa_auth_config *conf = &sm->wpa_auth->conf;
	size_t len = wpa_cipher_key_len(conf->group_mgmt_cipher);

	if (!sm->mgmt_frame_prot)
		return pos;

	igtk.keyid[0] = gsm->GN_igtk;
	igtk.keyid[1] = 0;
	if (gsm->wpa_group_state != WPA_GROUP_SETKEYSDONE ||
	    ml_get_seqnum(auth, NULL, gsm->GN_igtk, rsc) < 0)
		os_memset(igtk.pn, 0, sizeof(igtk.pn));
	else
		os_memcpy(igtk.pn, rsc, sizeof(igtk.pn));
	os_memcpy(igtk.igtk, gsm->IGTK[gsm->GN_igtk - 4], len);
	if (conf->disable_gtk || sm->wpa_key_mgmt == WPA_KEY_MGMT_OSEN) {
		/*
		 * Provide unique random IGTK to each STA to prevent use of
		 * IGTK in the BSS.
		 */
		if (random_get_bytes(igtk.igtk, len) < 0)
			return pos;
	}
	igtk.info = (link->link_id & 0x0f) << 4;
	ori = pos;
	pos = wpa_add_kde(pos, RSN_KEY_DATA_MLO_IGTK,
			  (const u8 *) &igtk, WPA_MLO_IGTK_KDE_PREFIX_LEN + len,
			  NULL, 0);
	wpa_hexdump_key(MSG_DEBUG, "ML: IGTK KDE", ori, pos - ori);

	if (!conf->beacon_prot)
		return pos;

	bigtk.keyid[0] = gsm->GN_bigtk;
	bigtk.keyid[1] = 0;
	if (gsm->wpa_group_state != WPA_GROUP_SETKEYSDONE ||
	    ml_get_seqnum(auth, NULL, gsm->GN_bigtk, rsc) < 0)
		os_memset(bigtk.pn, 0, sizeof(bigtk.pn));
	else
		os_memcpy(bigtk.pn, rsc, sizeof(bigtk.pn));
	os_memcpy(bigtk.bigtk, gsm->BIGTK[gsm->GN_bigtk - 6], len);
	if (sm->wpa_key_mgmt == WPA_KEY_MGMT_OSEN) {
		/*
		 * Provide unique random BIGTK to each OSEN STA to prevent use
		 * of BIGTK in the BSS.
		 */
		if (random_get_bytes(bigtk.bigtk, len) < 0)
			return pos;
	}
	bigtk.info = (link->link_id & 0x0f) << 4;
	ori = pos;
	pos = wpa_add_kde(pos, RSN_KEY_DATA_MLO_BIGTK,
			  (const u8 *) &bigtk, WPA_MLO_BIGTK_KDE_PREFIX_LEN + len,
			  NULL, 0);
	wpa_hexdump(MSG_DEBUG, "ML: BIGTK KDE", ori, pos - ori);

	return pos;
}

u8* ml_add_rekey_kde(struct wpa_state_machine *sm, u8 *pos)
{
	struct wpa_ml_group *ml_group = NULL;
	struct wpa_ml_link *link;
	u8 i;

	if (!sm->dot11MultiLinkActivated)
		return pos;

	wpa_printf(MSG_INFO, "ML: Add Mac/GTK into EAPOL-Key rekey");
	ml_group = STATE_MACHINE_ML_GROUP;
	pos = ml_set_mac_kde(pos, ml_group->ml_addr);

	for (i = 0; i < ml_group->ml_link_num; i++) {
		link = &ml_group->links[i];
		pos = ml_set_gtk_kde(sm, pos, link);
		pos = ml_set_ieee80211w_kde(sm, pos, link);
	}

	return pos;
}

int ml_rekey_gtk(struct wpa_state_machine *sm, struct wpa_eapol_ie_parse *kde)
{
	if (sm->dot11MultiLinkActivated &&
	    os_memcmp(kde->mac_addr, sm->sta_ml_ie->ml_addr, ETH_ALEN) == 0) {
		struct wpa_ml_group *ml_group;
		size_t i;

		wpa_auth_logger(sm->wpa_auth, sm->addr, LOGGER_INFO,
			"received EAPOL-Key Request for ML GTK rekeying");

		ml_group = STATE_MACHINE_ML_GROUP;
		for (i = 0; i < ml_group->ml_link_num; i++) {
			struct wpa_authenticator *wpa_auth =
				((struct hostapd_data *)ml_group->links[i].ctx)->wpa_auth;

			eloop_cancel_timeout(wpa_rekey_gtk, wpa_auth, NULL);
			wpa_rekey_gtk(wpa_auth,	NULL);
		}
	}
	return 0;
}


