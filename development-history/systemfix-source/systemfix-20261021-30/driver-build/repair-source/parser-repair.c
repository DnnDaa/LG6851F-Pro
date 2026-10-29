#include "rt_config.h"
#define defrag_multi_link_per_sta_profile lg6_defrag_multi_link_per_sta_profile
#define parse_multi_link_per_sta_profile lg6_parse_multi_link_per_sta_profile
#define parse_multi_link_ie lg6_parse_multi_link_ie
/* Bounded replacement parsers. No capability or mode is disabled here. */
u16 defrag_multi_link_per_sta_profile(u8 *input, u8 *end, u8 *output, u16 *out_len)
{
	u32 wire = 0, used = 0;
	u8 len;
	if (!input || !end || !output || !out_len || end < input)
		return 0;
	*out_len = 0;
	if (end - input < 2 || input[0] != SUB_IE_MLD_PER_STA_PROFILE)
		return 0;
	len = input[1];
	if (end - input < 2 + len || 2 + len > BSS_MNGR_MAX_PER_STA_PROFILE_LEN)
		return 0;
	wire = used = 2 + len;
	NdisMoveMemory(output, input, used);
	while (len == MAX_LEN_OF_IE_DATA && wire < end - input) {
		if (end - input - wire < 2)
			return 0;
		if (input[wire] != SUB_IE_MLD_FRAGMENT)
			break;
		len = input[wire + 1];
		if (!len || end - input - wire < 2 + len ||
		    len > BSS_MNGR_MAX_PER_STA_PROFILE_LEN - used || wire + 2 + len > 65535)
			return 0;
		NdisMoveMemory(output + used, input + wire + 2, len);
		used += len;
		wire += 2 + len;
	}
	*out_len = used;
	return wire - 2;
}

int parse_multi_link_per_sta_profile(u8 *input, u16 len, struct ml_ie_info *info)
{
	u8 *pos, *end, *info_end;
	u16 ctrl;
	u8 id, info_len;
	struct ml_ie_link *link;
	if (!input || !info || len < 2)
		return -1;
	pos = input + 2;
	end = pos + len;
	NdisMoveMemory(&ctrl, pos, 2);
	ctrl = le16_to_cpu(ctrl);
	pos += 2;
	id = GET_DOT11BE_ML_PER_STA_CTRL_LINK_ID(ctrl);
	if (id >= BSS_MNGR_MAX_BAND_NUM)
		return 0; /* Unsupported link is not an array index. */
	link = &info->link[id];
	link->active = FALSE;
	link->sta_profile_len = 0;
	link->sta_ctrl = ctrl;
	if (info->type == ML_CTRL_TYPE_BASIC || info->type == ML_CTRL_TYPE_RECONFIG) {
		if (pos == end)
			return -1;
		info_len = *pos;
		/* Length includes its own octet, not the two-octet STA Control. */
		if (!info_len || info_len > end - pos)
			return -1;
		info_end = pos + info_len;
		link->sta_info_len = info_len;
		pos++;
		if (info->type == ML_CTRL_TYPE_RECONFIG) {
			link->link_id = id;
			link->reconfig_type = GET_DOT11BE_ML_RECONFIG_PER_STA_CTRL_OP_UPD_TYPE(ctrl);
			link->active = TRUE;
			return 0;
		}
#define LG_STA_TAKE(dst, count) do { \
	if ((count) > info_end - pos) return -1; \
	NdisMoveMemory((dst), pos, (count)); pos += (count); \
} while (0)
		if (GET_DOT11BE_ML_PER_STA_CTRL_MAC_ADDR_PRES(ctrl))
			LG_STA_TAKE(link->link_addr, MAC_ADDR_LEN);
		if (GET_DOT11BE_ML_PER_STA_CTRL_BCN_INT_PRES(ctrl)) {
			LG_STA_TAKE(&link->bcn_interval, 2);
			link->bcn_interval = le16_to_cpu(link->bcn_interval);
		}
		if (GET_DOT11BE_ML_PER_STA_CTRL_TSF_OFFSET_PRES(ctrl)) {
			if (info_end - pos < 8) return -1;
			pos += 8;
		}
		if (GET_DOT11BE_ML_PER_STA_CTRL_DTIM_PRES(ctrl)) {
			LG_STA_TAKE(&link->dtim_count, 1);
			LG_STA_TAKE(&link->dtim_period, 1);
		}
		if (GET_DOT11BE_ML_PER_STA_CTRL_COMP_PROF(ctrl) &&
		    GET_DOT11BE_ML_PER_STA_CTRL_NSTR_LPR_PRES(ctrl)) {
			link->nstr_bmap = 0;
			LG_STA_TAKE(&link->nstr_bmap,
				GET_DOT11BE_ML_PER_STA_CTRL_NSTR_BMAP_SIZE(ctrl) ? 2 : 1);
			link->nstr_bmap = le16_to_cpu(link->nstr_bmap);
		}
		if (GET_DOT11BE_ML_PER_STA_CTRL_BSS_CHG_CNT_PRES(ctrl)) {
			if (pos == info_end) return -1;
			pos++;
		}
#undef LG_STA_TAKE
		/* Unknown future STA Info fields are skipped using the declared length. */
		pos = info_end;
	}
	if (end - pos > sizeof(link->sta_profile))
		return -1;
	link->sta_profile_len = end - pos;
	NdisMoveMemory(link->sta_profile, pos, link->sta_profile_len);
	link->active = TRUE;
	return 0;
}

int parse_multi_link_ie(u8 *input, struct frag_ie_info *frag, struct ml_ie_info *info)
{
	u8 *pos, *end, *common_end, *buf = NULL;
	u16 len, ctrl, presence, plen, wire;
	if (!input || !info)
		return -1;
	NdisZeroMemory(info, sizeof(*info));
	if (input[0] != IE_WLAN_EXTENSION)
		return -1;
	if (frag && frag->is_frag) {
		if (frag->ie_len_defrag < 2) return -1;
		len = frag->ie_len_defrag - 2;
	} else
		len = input[1];
	if (len < 3 || input[2] != EID_EXT_EHT_MULTI_LINK)
		return -1;
	pos = input + 3;
	end = input + 2 + len;
	NdisMoveMemory(&ctrl, pos, 2);
	ctrl = le16_to_cpu(ctrl);
	pos += 2;
	info->type = GET_DOT11BE_ML_CTRL_TYPE(ctrl);
	info->pres_bmap = presence = GET_DOT11BE_ML_CTRL_PRESENCE_BMAP(ctrl);
	common_end = end;
#ifdef DOT11_BE_D12
	if (pos == end || !*pos || *pos > end - pos)
		goto bad;
	info->cmm_info_len = *pos;
	common_end = pos + *pos;
	pos++;
#endif
#define LG_ML_TAKE(dst, count) do { \
	if ((count) > common_end - pos) goto bad; \
	NdisMoveMemory((dst), pos, (count)); pos += (count); \
} while (0)
#define LG_ML_LE16(dst) do { LG_ML_TAKE(&(dst), 2); (dst) = le16_to_cpu(dst); } while (0)
	switch (info->type) {
	case ML_CTRL_TYPE_BASIC:
		LG_ML_TAKE(info->mld_addr, MAC_ADDR_LEN);
		if (presence & DOT11BE_ML_BASIC_PRESENCE_LINK_ID)
			LG_ML_TAKE(&info->link_id, 1);
		if (presence & DOT11BE_ML_BASIC_PRESENCE_BSS_CHG_CNT)
			LG_ML_TAKE(&info->bss_chg_cnt, 1);
		if (presence & DOT11BE_ML_BASIC_PRESENCE_MD_SYNC_DELAY)
			LG_ML_LE16(info->md_sync_delay);
		if (presence & DOT11BE_ML_BASIC_PRESENCE_EML_CAPS) {
			LG_ML_LE16(info->eml_caps);
			info->emlsr_sup = GET_DOT11BE_ML_BASIC_CMM_EML_CAP_EMLSR_SUP(info->eml_caps);
		}
		if (presence & DOT11BE_ML_BASIC_PRESENCE_MLD_CAPS_N_OP)
			LG_ML_LE16(info->mld_caps);
		if (presence & DOT11BE_ML_BASIC_PRESENCE_MLD_ID)
			LG_ML_TAKE(&info->mld_id, 1);
		if (presence & DOT11BE_ML_BASIC_PRESENCE_EXT_MLD_CAPS_N_OP)
			LG_ML_LE16(info->ext_mld_caps);
		break;
	case ML_CTRL_TYPE_PROBE_REQ:
		if (presence & DOT11BE_ML_PROBE_REQ_PRESENCE_MLD_ID)
			LG_ML_TAKE(&info->mld_id, 1);
		break;
	case ML_CTRL_TYPE_RECONFIG:
		if (presence & DOT11BE_ML_RECONFIG_PRESENCE_MLD_MAC_ADDR)
			LG_ML_TAKE(info->mld_addr, MAC_ADDR_LEN);
		break;
	default:
		goto bad;
	}
#undef LG_ML_TAKE
#undef LG_ML_LE16
#ifdef DOT11_BE_D12
	pos = common_end;
#endif
	while (pos < end) {
		if (end - pos < 2 || pos[1] > end - pos - 2)
			goto bad;
		wire = pos[1];
		if (pos[0] == SUB_IE_MLD_PER_STA_PROFILE) {
			if (pos[1] == MAX_LEN_OF_IE_DATA) {
				if (!buf)
					os_alloc_mem(NULL, (UCHAR **)&buf, BSS_MNGR_MAX_PER_STA_PROFILE_LEN);
				if (!buf) goto bad;
				wire = defrag_multi_link_per_sta_profile(pos, end, buf, &plen);
				if (!wire || plen < 2 || parse_multi_link_per_sta_profile(buf, plen - 2, info))
					goto bad;
			} else if (parse_multi_link_per_sta_profile(pos, pos[1], info))
				goto bad;
		}
		pos += 2 + wire;
	}
	if (buf) os_free_mem(buf);
	return 0;
bad:
	if (buf) os_free_mem(buf);
	NdisZeroMemory(info, sizeof(*info));
	return -1;
}
