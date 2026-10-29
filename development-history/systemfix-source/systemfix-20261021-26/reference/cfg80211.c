static int CFG80211_OpsStopAp(
	struct wiphy *pWiphy,
	struct net_device *netdev
#if defined(CFG_CFG80211_VERSION) && (KERNEL_VERSION(6, 1, 0) <= CFG_CFG80211_VERSION)
	, unsigned int link_id
#endif /* CFG_CFG80211_VERSION */
	)
{
	struct _RTMP_ADAPTER *pAd;
	INT apidx, i;
	struct wifi_dev *pWdev = NULL;
	struct _NDIS_AP_802_11_PMKID *pmkid_cache;
#ifdef DOT11_EHT_BE
	UCHAR *own_mac_mld = NULL;
#endif /*DOT11_EHT_BE*/

	MAC80211_PAD_GET(pAd, pWiphy);
	MTWF_DBG(NULL, DBG_CAT_CFG80211, CATCFG80211_AP, DBG_LVL_ERROR, "80211> ==>\n");
	apidx = CFG80211_FindMbssApIdxByNetDevice(pAd, netdev);
	pmkid_cache = PD_GET_PMKID_PTR(pAd->physical_dev);

	if (!VALID_MBSS(pAd, apidx)) {
		MTWF_DBG(NULL, DBG_CAT_CFG80211, CATCFG80211_AP, DBG_LVL_ERROR,
			"80211> wdev not found<==\n");
		return -ENODEV;
	}
#ifdef DOT11_EHT_BE
	MBSS_Reconfig_Deinit(pAd, &pAd->ApCfg.MBSSID[apidx]);
#endif
	pWdev = &pAd->ApCfg.MBSSID[apidx].wdev;
	RTMP_DRIVER_80211_BEACON_DEL(pAd, apidx);
#ifdef DOT11_EHT_BE
	/* sync ML Probe.rsp Per-STA Profile buffer */
	bss_mngr_mld_sync_ml_probe_rsp(pWdev);
	own_mac_mld = pWdev->bss_info_argument.mld_info.mld_addr;
#endif /* DOT11_EHT_BE */
	/* flush all pmk cache while ap stop*/
	OS_SEM_LOCK(&pmkid_cache->pmkid_lock);
	MTWF_DBG(NULL, DBG_CAT_CFG80211, CATCFG80211_AP, DBG_LVL_NOTICE,
		"80211> flush pmkid cache of bss ("MACSTR")<==\n",
		MAC2STR(pWdev->if_addr));
	for (i = 0; i < MAX_PMKID_COUNT; i++) {
		RTMPDeletePMKIDCache(pmkid_cache, pWdev->if_addr, i);
#ifdef DOT11_EHT_BE
		if (!MAC_ADDR_EQUAL(own_mac_mld, pWdev->if_addr) && !MAC_ADDR_EQUAL(own_mac_mld, ZERO_MAC_ADDR)) {
			MTWF_DBG(NULL, DBG_CAT_SEC, CATSEC_KEY, DBG_LVL_INFO,
				"clear PMKID cache for MLO, own mac mld="MACSTR"\n", own_mac_mld);
			RTMPDeletePMKIDCache(pmkid_cache, own_mac_mld, i);
		}
#endif /* DOT11_EHT_BE */
	}
	OS_SEM_UNLOCK(&pmkid_cache->pmkid_lock);
	return 0;

}