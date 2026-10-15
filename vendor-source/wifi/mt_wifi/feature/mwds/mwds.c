/*
 * Copyright (c) [2020], MediaTek Inc. All rights reserved.
 *
 * This software/firmware and related documentation ("MediaTek Software") are
 * protected under relevant copyright laws.
 * The information contained herein is confidential and proprietary to
 * MediaTek Inc. and/or its licensors.
 * Except as otherwise provided in the applicable licensing terms with
 * MediaTek Inc. and/or its licensors, any reproduction, modification, use or
 * disclosure of MediaTek Software, and information contained herein, in whole
 * or in part, shall be strictly prohibited.
*/
/*
 ***************************************************************************
 ***************************************************************************


    Module Name:
    mwds.c

    Abstract:
    This is MWDS feature used to process those 4-addr of connected APClient or STA.

    Revision History:
    Who          When          What
    ---------    ----------    ----------------------------------------------
 */
#ifdef MWDS
#include "rt_config.h"

#ifdef DOT11_EHT_BE
VOID mlo_mwds_ap_sync(
	IN PRTMP_ADAPTER pAd,
	IN PMAC_TABLE_ENTRY pEntry
)
{
	UCHAR i = 0;
	struct _MAC_TABLE_ENTRY *entry_ptr;
	struct mld_entry_t *mld_entry = NULL;

	if (!pAd || !pEntry) {
		MTWF_DBG(pAd, DBG_CAT_MLO, CATMLO_WDS, DBG_LVL_ERROR,
			"mwds NULL!\n");
		return;
	}
	if (pEntry && pEntry->mlo.mlo_en) {
		mt_rcu_read_lock();
		mld_entry = rcu_dereference(pEntry->mld_entry);
		if (mld_entry) {
			do {
				entry_ptr = mld_entry->link_entry[i++];
				if (!entry_ptr)
					continue;
				entry_ptr->bSupportMWDS = TRUE;
			} while (i < MLD_LINK_MAX);
		} else {
			MTWF_DBG(pAd, DBG_CAT_MLO, CATMLO_WDS, DBG_LVL_ERROR,
				"mld_entry=NULL\n");
			mt_rcu_read_unlock();
			return;
		}
		mt_rcu_read_unlock();
	}
}

VOID mlo_mwds_apcli_sync(
	IN PSTA_ADMIN_CONFIG pApCliEntry,
	IN PMAC_TABLE_ENTRY pEntry
)
{
	UCHAR i = 0;
	struct _MAC_TABLE_ENTRY *entry_ptr;
	struct wifi_dev *wdev = &pApCliEntry->wdev;
	struct _RTMP_ADAPTER *ad;
	PSTA_ADMIN_CONFIG pStaCfg;
	struct mld_dev *mld = wdev->mld_dev;

	if (pEntry && pEntry->mlo.mlo_en) {
		for (i = 0; i < MLD_LINK_MAX; i++) {
			entry_ptr = (MAC_TABLE_ENTRY *)mld->peer_mld.single_link[i].priv_ptr;
			if (!entry_ptr)
				continue;

			ad = entry_ptr->pAd;
			if (!ad)
				continue;

			pStaCfg = GetStaCfgByWdev(ad, mld->mld_own_links[i].wdev);
			if (!pStaCfg)
				continue;
			pStaCfg->MlmeAux.bSupportMWDS = TRUE;
		}
	}
}
#endif

VOID MWDSAPPeerEnable(
	IN PRTMP_ADAPTER pAd,
	IN PMAC_TABLE_ENTRY pEntry
)
{
	BOOLEAN mwds_enable = FALSE;
	BOOLEAN Ret = FALSE;
#ifdef DOT11_EHT_BE
	UCHAR i;
	struct _MAC_TABLE_ENTRY *entry_ptr;
	struct _RTMP_ADAPTER *ad;
	struct mld_entry_t *mld_entry = NULL;
#endif

	if (pEntry->bSupportMWDS && pEntry->wdev && pEntry->wdev->bSupportMWDS)
		mwds_enable = TRUE;

#ifdef CONFIG_MAP_SUPPORT
/*	MAP have higher priority,
*	If MAP is enabled and peer have MAP capability as well,
*	use MAP connection and disable MWDS
*/

	if (IS_MAP_ENABLE(pAd) &&
		(pEntry->DevPeerRole & BIT(MAP_ROLE_BACKHAUL_STA)))
		mwds_enable = FALSE;
#endif

	if (mwds_enable) {
#ifdef DOT11_EHT_BE
		i = 0;
		if (pEntry->mlo.mlo_en) {
			if (!pEntry->mlo.is_setup_link_entry)
				return;

			mt_rcu_read_lock();
			mld_entry = rcu_dereference(pEntry->mld_entry);
			if (mld_entry) {
				do {
					entry_ptr = mld_entry->link_entry[i++];
					if (!entry_ptr)
						continue;
					ad = entry_ptr->pAd;
					if (!ad)
						continue;

					Ret = a4_ap_peer_enable(ad, entry_ptr, A4_TYPE_MWDS);
					if (Ret == FALSE) {
						MWDSAPPeerDisable(ad, entry_ptr);
						mt_rcu_read_unlock();
						return;
					}
				} while (i < MLD_LINK_MAX);
			}
			mt_rcu_read_unlock();
		} else
#endif
		{
			Ret = a4_ap_peer_enable(pAd, pEntry, A4_TYPE_MWDS);
			if (Ret == FALSE)
				MWDSAPPeerDisable(pAd, pEntry);
		}
	}
}

VOID MWDSAPPeerDisable(
	IN PRTMP_ADAPTER pAd,
	IN PMAC_TABLE_ENTRY pEntry
)
{
	BOOLEAN Ret;

	Ret = a4_ap_peer_disable(pAd, pEntry, A4_TYPE_MWDS);
	if (Ret)
		pEntry->bSupportMWDS = FALSE;
}

#ifdef APCLI_SUPPORT
VOID MWDSAPCliPeerEnable(
	IN PRTMP_ADAPTER pAd,
	IN PSTA_ADMIN_CONFIG pApCliEntry,
	IN PMAC_TABLE_ENTRY pEntry
)
{
	BOOLEAN mwds_enable = FALSE;
	BOOLEAN Ret = FALSE;

	if (pApCliEntry->MlmeAux.bSupportMWDS && pApCliEntry->wdev.bSupportMWDS)
		mwds_enable = TRUE;

	if (pApCliEntry->wdev.wps_ie_flag)
		mwds_enable = FALSE;

#ifdef CONFIG_MAP_SUPPORT
/*	MAP have higher priority
*	If MAP is enabled and peer have MAP capability as well,
*	use MAP connection and disable MWDS
*/
	if (IS_MAP_ENABLE(pAd) &&
		(pEntry->DevPeerRole &
			(BIT(MAP_ROLE_FRONTHAUL_BSS) | BIT(MAP_ROLE_BACKHAUL_BSS))))
		mwds_enable = FALSE;
#endif

	if (mwds_enable) {
#ifdef DOT11_EHT_BE
		/* mlo case */
		if (pEntry && pEntry->mlo.mlo_en) {
			struct wifi_dev *wdev = &pApCliEntry->wdev;
			struct mld_dev *mld = wdev->mld_dev;
			UCHAR link;
			struct _MAC_TABLE_ENTRY *entry_ptr;
			struct _RTMP_ADAPTER *ad;
			PSTA_ADMIN_CONFIG pStaCfg;

			for (link = 0; link < BSS_MNGR_MAX_BAND_NUM; link++) {
				if (mld->peer_mld.single_link[link].active && mld->mld_own_links[link].used) {
					entry_ptr = (MAC_TABLE_ENTRY *)mld->peer_mld.single_link[link].priv_ptr;
					if (!entry_ptr)
						continue;
					ad = entry_ptr->pAd;
					if (!ad)
						continue;

					pStaCfg = GetStaCfgByWdev(ad, mld->mld_own_links[link].wdev);
					Ret = a4_apcli_peer_enable(ad, pStaCfg, entry_ptr, A4_TYPE_MWDS);
					if (Ret == FALSE)
						MWDSAPCliPeerDisable(ad, pStaCfg, entry_ptr);
				}
			}
		} else
#endif
		{
			Ret = a4_apcli_peer_enable(pAd, pApCliEntry, pEntry, A4_TYPE_MWDS);
			if (Ret == FALSE)
				MWDSAPCliPeerDisable(pAd, pApCliEntry, pEntry);
		}
	}
}

#ifdef OPENSYNC_WDS
struct wds_tx_null_arg_t {
	IN PRTMP_ADAPTER pAd;
	IN PSTA_ADMIN_CONFIG pApCliEntry;
	IN PMAC_TABLE_ENTRY pEntry;
};

int wds_update_callback_after_tx_null(
	void *arg,
	struct txs_info_t *txs_info
)
{
	int ret = 0;
	struct wds_tx_null_arg_t *callback_arg = NULL;

	if (!arg || !txs_info) {
		ret = -EINVAL;
		goto err1;
	}

	callback_arg = arg;

	if (txs_info->txs_sts == TXS_STS_OK) {
		STA_TR_ENTRY *tr_entry  = NULL;
		PRTMP_ADAPTER pAd = callback_arg->pAd;
		PSTA_ADMIN_CONFIG pApCliEntry = callback_arg->pApCliEntry;
		PMAC_TABLE_ENTRY pEntry = callback_arg->pEntry;

		if (!pAd || !pApCliEntry || !pEntry) {
			ret = -1;
			goto err1;
		}
		MTWF_PRINT("send 4-address null success!\n");
		pApCliEntry->MlmeAux.bSupportMWDS = 1;
		tr_entry = tr_entry_get(pAd, pEntry->wcid);
		if (tr_entry && (tr_entry->PortSecured == WPA_802_1X_PORT_SECURED))
			MWDSAPCliPeerEnable(pAd, pApCliEntry, pEntry);

	} else if (txs_info->txs_sts == TXS_STS_NG) {
		MTWF_DBG(NULL, DBG_CAT_AP, CATAP_WDS, DBG_LVL_ERROR,
			 "TXS_STS_NG\n");
		ret = -EINVAL;
		goto err2;
	} else if (txs_info->txs_sts == TXS_STS_TO) {
		MTWF_DBG(NULL, DBG_CAT_AP, CATAP_WDS, DBG_LVL_ERROR,
			 "TXS_STS_TO\n");
		ret = -EINVAL;
		goto err2;
	}

	return ret;

err1:
	MTWF_DBG(NULL, DBG_CAT_AP, CATAP_WDS, DBG_LVL_ERROR,
		 "err=%d\n", ret);
	return ret;
err2:
	MTWF_DBG(NULL, DBG_CAT_AP, CATAP_WDS, DBG_LVL_ERROR,
		 "err=%d, txs_sts=%d\n", ret, txs_info->txs_sts);
	return ret;
}

int wds_trigger_4addr_conn(
	IN PRTMP_ADAPTER pAd,
	IN PSTA_ADMIN_CONFIG pApCliEntry,
	IN PMAC_TABLE_ENTRY pEntry
)
{
	int ret = 0;
	struct _HEADER_802_11_A4 *pNullFr;
	UCHAR *pFrame;
	NDIS_STATUS NState;
	struct txs_callback_info_t *callback = NULL;
	struct wds_tx_null_arg_t *callback_arg = NULL;

	if (!pApCliEntry || !pEntry) {
		ret = -1;
		return ret;
	}

	if (!pApCliEntry->wdev.bSupportMWDS) {
		ret = -1;
		return ret;
	}

	NState = MlmeAllocateMemory(pAd, (UCHAR **)&pFrame);
	pNullFr = (struct _HEADER_802_11_A4 *) pFrame;
	if (NState == NDIS_STATUS_SUCCESS) {
		UINT frm_len;

		frm_len = sizeof(struct _HEADER_802_11_A4);
		NdisZeroMemory(pNullFr, frm_len);
		COPY_MAC_ADDR(pNullFr->Addr1, pEntry->Addr);
		COPY_MAC_ADDR(pNullFr->Addr2, pEntry->wdev->if_addr);
		COPY_MAC_ADDR(pNullFr->Addr3, pEntry->Addr);
		COPY_MAC_ADDR(pNullFr->Addr4, pEntry->wdev->if_addr);
		pNullFr->FC.ToDs = 1;
		pNullFr->FC.FrDs = 1;
		pNullFr->FC.Type = FC_TYPE_DATA;
		pNullFr->FC.SubType = SUBTYPE_DATA_NULL;
		pNullFr->Duration = RTMPCalcDuration(pAd, pEntry->CurrTxRate, frm_len);
		if (TxsInitCallbackInfo(pAd,
			&callback,
			wds_update_callback_after_tx_null,
			(VOID **)&callback_arg,
			sizeof(struct wds_tx_null_arg_t))
			== NDIS_STATUS_SUCCESS) {
			callback_arg->pAd = pAd;
			callback_arg->pEntry = pEntry;
			callback_arg->pApCliEntry = pApCliEntry;
		} else {
			MlmeFreeMemory(pFrame);
			ret = -1;
			goto err;
		}
		MiniportMMRequest(pAd,
			NULL_USE_ALTX_FLAG | WMM_UP2AC_MAP[7], (PUCHAR)pNullFr, frm_len, callback);
		MlmeFreeMemory(pFrame);
	}
	return ret;

err:
	MTWF_DBG(NULL, DBG_CAT_AP, CATAP_WDS, DBG_LVL_ERROR,
		 "err=%d\n", ret);
	return ret;
}

VOID opensync_wds_action(IN PRTMP_ADAPTER pAd, IN MLME_QUEUE_ELEM * Elem)
{

	struct wifi_dev *wdev = NULL;
	PFRAME_802_11 pFrame = NULL;
	MAC_TABLE_ENTRY *pEntry = NULL;

	if (!Elem || !Elem->wdev)
		return;
	pFrame =  (PFRAME_802_11)(Elem->Msg);
	wdev = Elem->wdev;
	pEntry = MacTableLookup2(pAd, pFrame->Hdr.Addr2, wdev);
	if (pEntry)
		MWDSAPPeerEnable(pAd, pEntry);
}

#endif

VOID MWDSAPCliPeerDisable(
	IN PRTMP_ADAPTER pAd,
	IN PSTA_ADMIN_CONFIG pApCliEntry,
	IN PMAC_TABLE_ENTRY pEntry
)
{
#ifdef DOT11_EHT_BE
	if (pEntry && pEntry->mlo.mlo_en) {
		UCHAR i;
		struct _MAC_TABLE_ENTRY *entry_ptr;
		struct wifi_dev *wdev = &pApCliEntry->wdev;
		struct _RTMP_ADAPTER *ad;
		PSTA_ADMIN_CONFIG pStaCfg;
		struct mld_dev *mld = wdev->mld_dev;

		for (i = 0; i < MLD_LINK_MAX; i++) {
			entry_ptr = (MAC_TABLE_ENTRY *)mld->peer_mld.single_link[i].priv_ptr;
			if (!entry_ptr)
				continue;

			ad = entry_ptr->pAd;
			if (!ad)
				continue;

			pStaCfg = GetStaCfgByWdev(ad, mld->mld_own_links[i].wdev);
			if (!pStaCfg)
				continue;

			a4_apcli_peer_disable(ad, pStaCfg, entry_ptr, A4_TYPE_MWDS);
		}
	} else
#endif
		a4_apcli_peer_disable(pAd, pApCliEntry, pEntry, A4_TYPE_MWDS);
}
#endif /* APCLI_SUPPORT */

#ifdef RT_CFG80211_SUPPORT
INT mtk_cfg80211_set_mwds_cap(
	IN PRTMP_ADAPTER pAd,
	IN UCHAR ifIndex,
	IN BOOLEAN isAP,
	IN UCHAR enable
)
{
	struct wifi_dev *wdev = NULL;

	if (isAP) {
		pAd->ApCfg.MBSSID[ifIndex].wdev.bDefaultMwdsStatus = (enable == 0) ? FALSE : TRUE;

		if (ifIndex < MAX_BEACON_NUM) {
#ifdef DOT11_EHT_BE
			struct wifi_dev *wdev = &pAd->ApCfg.MBSSID[ifIndex].wdev;

			bss_mngr_mld_mwds_enable(wdev, enable);
#else
			if (enable)
				MWDSEnable(pAd, ifIndex, TRUE, FALSE);
			else
				MWDSDisable(pAd, ifIndex, TRUE, FALSE);
#endif
		}

	} else {
#ifdef DOT11_EHT_BE
		UCHAR link_idx;
		UCHAR if_idx;
		struct mld_link_entry *link;
		struct _RTMP_ADAPTER *ad;

		if (ifIndex >= MAX_APCLI_NUM)
			return FALSE;

		wdev = &pAd->StaCfg[ifIndex].wdev;
		if (IS_APCLI_DISABLE_MLO(wdev)) {
			if (enable)
				MWDSEnable(pAd, wdev->func_idx, FALSE, FALSE);
			else
				MWDSDisable(pAd, wdev->func_idx, FALSE, FALSE);
		} else {
			for (link_idx = 0; link_idx < MLD_LINK_MAX; link_idx++) {
				link = get_sta_mld_link_by_idx(wdev->mld_dev, link_idx);
				if (!link || !link->wdev)
					continue;

				ad = (struct _RTMP_ADAPTER *)link->wdev->sys_handle;
				if (!ad)
					continue;

				if_idx = link->wdev->func_idx;
				if (enable)
					MWDSEnable(ad, if_idx, FALSE, FALSE);
				else
					MWDSDisable(ad, if_idx, FALSE, FALSE);
			}
		}
#else
		if (enable)
			MWDSEnable(pAd, ifIndex, FALSE, FALSE);
		else
			MWDSDisable(pAd, ifIndex, FALSE, FALSE);
#endif
		pAd->StaCfg[ifIndex].wdev.bDefaultMwdsStatus = (enable == 0) ? FALSE : TRUE;
	}

	return TRUE;
}
#endif
INT MWDSEnable(
	IN PRTMP_ADAPTER pAd,
	IN UCHAR ifIndex,
	IN BOOLEAN isAP,
	IN BOOLEAN isDevOpen
)
{
	struct wifi_dev *wdev = NULL;

	if (isAP) {
		if (ifIndex < MAX_BEACON_NUM) {
			wdev = &pAd->ApCfg.MBSSID[ifIndex].wdev;

			if (!wdev->bSupportMWDS) {
				wdev->bSupportMWDS = TRUE;
				a4_interface_init(pAd, ifIndex, TRUE, A4_TYPE_MWDS);

				if (!isDevOpen)
					UpdateBeaconHandler(pAd, wdev, BCN_REASON(BCN_UPDATE_IE_CHG));
			}
		}
	}

#ifdef APCLI_SUPPORT
	else {
		if (ifIndex < MAX_APCLI_NUM) {
			wdev = &pAd->StaCfg[ifIndex].wdev;

			if (!wdev->bSupportMWDS) {
				wdev->bSupportMWDS = TRUE;
				a4_interface_init(pAd, ifIndex, FALSE, A4_TYPE_MWDS);
			}
		}
	}

#endif /* APCLI_SUPPORT */

	return TRUE;
}

INT MWDSDisable(
	IN PRTMP_ADAPTER pAd,
	IN UCHAR ifIndex,
	IN BOOLEAN isAP,
	IN BOOLEAN isDevClose
)
{
	struct wifi_dev *wdev = NULL;

	if (isAP) {
		if (ifIndex < MAX_BEACON_NUM) {
			wdev = &pAd->ApCfg.MBSSID[ifIndex].wdev;

			if (wdev && wdev->bSupportMWDS) {
				wdev->bSupportMWDS = FALSE;
				a4_interface_deinit(pAd, ifIndex, TRUE, A4_TYPE_MWDS);

				if (!isDevClose)
					UpdateBeaconHandler(pAd, wdev, BCN_REASON(BCN_UPDATE_IE_CHG));
			}
		}
	}

#ifdef APCLI_SUPPORT
	else {
		if (ifIndex < MAX_APCLI_NUM) {
			wdev = &pAd->StaCfg[ifIndex].wdev;

			if (wdev && wdev->bSupportMWDS) {
				wdev->bSupportMWDS = FALSE;
				a4_interface_deinit(pAd, ifIndex, FALSE, A4_TYPE_MWDS);
			}
		}
	}

#endif /* APCLI_SUPPORT */

	return TRUE;
}


INT Set_Enable_MWDS_Proc(
	IN  PRTMP_ADAPTER pAd,
	IN  BOOLEAN Enable,
	IN  BOOLEAN isAP
)
{
	POS_COOKIE      pObj;
	UCHAR           ifIndex;
	pObj = (POS_COOKIE) pAd->OS_Cookie;

	if (isAP) {
		ifIndex = pObj->ioctl_if;
		pAd->ApCfg.MBSSID[ifIndex].wdev.bDefaultMwdsStatus = (Enable == 0) ? FALSE : TRUE;
	}

#ifdef APCLI_SUPPORT
	else {
		if (pObj->ioctl_if_type != INT_APCLI)
			return FALSE;

		ifIndex = pObj->ioctl_if;
		pAd->StaCfg[ifIndex].wdev.bDefaultMwdsStatus = (Enable == 0) ? FALSE : TRUE;
	}

#endif /* APCLI_SUPPORT */

	if (Enable)
		MWDSEnable(pAd, ifIndex, isAP, FALSE);
	else
		MWDSDisable(pAd, ifIndex, isAP, FALSE);

	return TRUE;
}

INT Set_Ap_MWDS_Proc(
	IN  PRTMP_ADAPTER pAd,
	IN  PSTRING arg
)
{
	UCHAR Enable;
	Enable = simple_strtol(arg, 0, 10);
	return Set_Enable_MWDS_Proc(pAd, Enable, TRUE);
}

INT Set_ApCli_MWDS_Proc(
	IN  PRTMP_ADAPTER pAd,
	IN  PSTRING arg
)
{
	UCHAR Enable;
	Enable = simple_strtol(arg, 0, 10);
	return Set_Enable_MWDS_Proc(pAd, Enable, FALSE);
}

#endif /* MWDS */
