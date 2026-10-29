case CMD_RTPRIV_IOCTL_80211_BEACON_DEL: {
#ifndef DISABLE_HOSTAPD_BEACON
			if (pAd->cfg80211_ctrl.beacon_tail_buf != NULL) {
				os_free_mem(pAd->cfg80211_ctrl.beacon_tail_buf);
				pAd->cfg80211_ctrl.beacon_tail_buf = NULL;
			}
			pAd->cfg80211_ctrl.beacon_tail_len = 0;
#endif
	}
	break;

	case CMD_RTPRIV_IOCTL_80211_AP_KEY_ADD:
		CFG80211DRV_ApKeyAdd(pAd, pData);
		break;

	case CMD_RTPRIV_IOCTL_80211_RTS_THRESHOLD_ADD: {
		UCHAR i = 0;

		for (i = 0; i < WDEV_NUM_MAX; i++) {
			if (pAd->wde