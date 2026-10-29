VOID MBSS_Reconfig_Deinit(RTMP_ADAPTER *pAd, BSS_STRUCT *pMbss)
{
	BOOLEAN Cancelled;

	if (!pMbss->reconfig_state_init)
		return;
	MTWF_DBG(pAd, DBG_CAT_AP, CATAP_RECONF_FLOW, DBG_LVL_INFO,
		"\x1b[42mpMbss->wdev.wdev_idx=%d\x1b[m\n\r", pMbss->wdev.wdev_idx);
	RTMPReleaseTimer(&pMbss->btm_timer, &Cancelled);
	NdisFreeSpinLock(&pMbss->reconfig_state_lock);

	pMbss->reconfig_state_init = FALSE;
}
