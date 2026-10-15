/*
 ***************************************************************************
 * MediaTek Inc.
 * 4F, No. 2 Technology 5th Rd.
 * Science-based Industrial Park
 * Hsin-chu, Taiwan, R.O.C.
 *
 * (c) Copyright 1997-2019, MediaTek, Inc.
 *
 * All rights reserved. MediaTek source code is an unpublished work and the
 * use of a copyright notice does not imply otherwise. This source code
 * contains confidential trade secret material of MediaTek. Any attempt
 * or participation in deciphering, decoding, reverse engineering or in any
 * way altering the source code is stricitly prohibited, unless the prior
 * written consent of MediaTek Technology, Inc. is obtained.
 ***************************************************************************

*/
#ifdef BELLWETHER

#include "rt_config.h"
#include "bellwether_ser.h"
#include "bellwether_coda/wf_cr_sw_def.h"
#include "bellwether_coda/wf_wfdma_host_dma0.h"

#ifdef ERR_RECOVERY
void bellwether_ser_1_0_v1(struct ser_cb_t *ser_cb)
{
	UINT32 chip_id = ser_cb->chip_id;
	UINT32 hw_id = ser_cb->hw_id;
	UINT8 level = ser_cb->ser_level;
	UINT8 state = ser_cb->ser_state;
	UINT32 event = ser_cb->ser_event;
	struct _RTMP_ADAPTER *pAd_list[MAX_NUM_OF_INF] = {NULL};
	struct _RTMP_ADAPTER *main_pAd, *pAd;
	ERR_RECOVERY_CTRL_T ErrRecoveryCtrl;
	UINT32 *pSerTimes = NULL;
	struct wifi_dev *wdev = NULL;
	int i, pAd_num;

	MTWF_DBG(NULL, DBG_CAT_HW, CATHW_SER, DBG_LVL_INFO,
		"chip_id=0x%x,hw_id=%d,level=%d,state=%d,event=0x%x\n",
		chip_id, hw_id, level, state, event);

	pAd_num = ser_get_all_pAd_by_chipid_hwid(pAd_list, chip_id, hw_id);
	main_pAd = pAd_list[0];

	if (pAd_num == 0 || main_pAd == NULL) {
		MTWF_DBG(NULL, DBG_CAT_HW, CATHW_SER, DBG_LVL_ERROR,
			"pAd_num is 0 or main_pAd is NULL\n");
		return;
	}

#ifdef CONFIG_ATE
	if (ATE_ON(main_pAd)) {
		MTWF_DBG(NULL, DBG_CAT_HW, CATHW_SER, DBG_LVL_ERROR,
			"The driver is in ATE mode now\n");
		return;
	}
#endif /* CONFIG_ATE */

	memset(&ErrRecoveryCtrl, 0, sizeof(ErrRecoveryCtrl));
	ErrRecoveryCtrl.errRecovStage = state;
	pSerTimes = &ser_cb->ser_times[0];
	MTWF_DBG(NULL, DBG_CAT_HW, CATHW_SER, DBG_LVL_DEBUG, "state=0x%08X\n", state);
	/*wlan hook for ser*/
	WLAN_HOOK_CALL(WLAN_HOOK_SER, main_pAd, &ErrRecoveryCtrl);

	switch (state) {
	case SER_LV_1_0_STATE_STOP_IDLE:
	case SER_LV_1_0_STATE_EVENT_REENTRY:
		if ((event & SER_LV_1_0_EVT_STOP_PDMA) == SER_LV_1_0_EVT_STOP_PDMA) {
			os_zero_mem(pSerTimes, (sizeof(pSerTimes[SER_TIME_ID_T0]) * SER_TIME_ID_END));
			pSerTimes[SER_TIME_ID_T0] = ser_get_time();
			ser_tans_ser_state(ser_cb, SER_LV_1_0_STATE_STOP_IDLE_DONE);

			all_pAd(i, pAd_list, pAd_num, pAd) {
				RTMP_CLEAR_FLAG(pAd, fRTMP_ADAPTER_START_UP | fRTMP_ADAPTER_MCU_SEND_IN_BAND_CMD);
				/* exit tasklet mgmt */
				tm_exit(pAd);
			}

			/* exit DMA_SW rxq queues */

			/* send PDMA0 stop to N9 through interrupt. */
			ser_ack_event(ser_cb, SER_LV_1_0_INT_PDMA0_STOP_DONE);

			/* all mmio need to be stop till hw reset done. */
			all_pAd(i, pAd_list, pAd_num, pAd)
				asic_ser_handler(pAd, SER_ACTION_IO_SWITCH, 0);

			RtmpusecDelay(100 * 1000); /* delay for 100 ms to wait reset done. */

			all_pAd(i, pAd_list, pAd_num, pAd)
				asic_ser_handler(pAd, SER_ACTION_IO_SWITCH, 1);

			/*re-call for change status to stop dma0*/
			bellwether_ser_1_0_v1(ser_cb);
			pSerTimes[SER_TIME_ID_T1] = ser_get_time();
		} else {
			MTWF_DBG(NULL, DBG_CAT_HW, CATHW_SER, DBG_LVL_ERROR,
				"!!! SER CurStat=%u Event=0x%x !!!\n",
				ser_get_curr_state(main_pAd), event);
		}

		break;

	case SER_LV_1_0_STATE_STOP_IDLE_DONE:
		ser_tans_ser_state(ser_cb, SER_LV_1_0_STATE_STOP_PDMA0);
		break;

	case SER_LV_1_0_STATE_STOP_PDMA0:
		if ((event & SER_LV_1_0_EVT_RESET_DONE) == SER_LV_1_0_EVT_RESET_DONE) {

			pSerTimes[SER_TIME_ID_T2] = ser_get_time();

			all_pAd(i, pAd_list, pAd_num, pAd)
				/* exit queue mgmt */
				qm_exit_perband(pAd);

			asic_ser_handler(main_pAd, SER_ACTION_SET_DMA_TK, 0);
			asic_ser_handler(main_pAd, SER_ACTION_SET_DMA_TK, 1);
			WLAN_HOOK_CALL(WLAN_HOOK_HIF_INIT, main_pAd, NULL);

			all_pAd(i, pAd_list, pAd_num, pAd) {
				/* init queue and tasklet mgmt */
				qm_init_perband(pAd);
				tm_init(pAd);
				/* restore flags */
				RTMP_SET_FLAG(pAd, fRTMP_ADAPTER_START_UP | fRTMP_ADAPTER_MCU_SEND_IN_BAND_CMD);
			}

			ser_tans_ser_state(ser_cb, SER_LV_1_0_STATE_RESET_PDMA0);

			/* send PDMA0 reinit done to N9 through interrupt. */
			ser_ack_event(ser_cb, SER_LV_1_0_INT_PDMA0_INIT_DONE);

			pSerTimes[SER_TIME_ID_T3] = ser_get_time();
		} else {
			MTWF_DBG(NULL, DBG_CAT_HW, CATHW_SER, DBG_LVL_ERROR,
				"!!! SER CurStat=%u Event=0x%x !!!\n",
				ser_get_curr_state(main_pAd), event);
		}

		break;

	case SER_LV_1_0_STATE_RESET_PDMA0:
		if ((event & SER_LV_1_0_EVT_RECOVERY_DONE)
			== SER_LV_1_0_EVT_RECOVERY_DONE) {
			pSerTimes[SER_TIME_ID_T4] = ser_get_time();
			ser_tans_ser_state(ser_cb, SER_LV_1_0_STATE_WAIT_N9_NORMAL);
			ser_ack_event(ser_cb, SER_LV_1_0_INT_PDMA0_RECV_DONE);
			pSerTimes[SER_TIME_ID_T5] = ser_get_time();
		} else {
			MTWF_DBG(NULL, DBG_CAT_HW, CATHW_SER, DBG_LVL_ERROR,
				"!!! SER CurStat=%u Event=0x%x !!!\n",
				ser_get_curr_state(main_pAd), event);
		}

		break;

	case SER_LV_1_0_STATE_WAIT_N9_NORMAL:
		if ((event & SER_LV_1_0_EVT_STOP_PDMA)
				   == SER_LV_1_0_EVT_STOP_PDMA) {
			pSerTimes[SER_TIME_ID_T6] = ser_get_time();
			MTWF_DBG(NULL, DBG_CAT_HW, CATHW_SER, DBG_LVL_ERROR,
				"!!! ERROR SER re-entry  CurStat=%u Event=0x%x !!!\n",
				ser_get_curr_state(main_pAd), event);
			pSerTimes[SER_TIME_ID_T7] = ser_get_time();

			/*print out ser log timing*/
			if (_ser_dump_timelog(ser_cb))
				MTWF_DBG(NULL, DBG_CAT_HW, CATHW_SER, DBG_LVL_ERROR,
					"SER dump timelog error\n");

			ser_tans_ser_state(ser_cb, SER_LV_1_0_STATE_EVENT_REENTRY);
			bellwether_ser_1_0_v1(ser_cb);
		} else if ((event & SER_LV_1_0_EVT_N9_NORMAL_STATE)
			== SER_LV_1_0_EVT_N9_NORMAL_STATE) {
			pSerTimes[SER_TIME_ID_T6] = ser_get_time();
			ser_tans_ser_state(ser_cb, SER_LV_1_0_STATE_STOP_IDLE);
			pSerTimes[SER_TIME_ID_T7] = ser_get_time();

			/*print out ser log timing*/
			if (_ser_dump_timelog(ser_cb))
				MTWF_DBG(NULL, DBG_CAT_HW, CATHW_SER, DBG_LVL_ERROR,
					"SER dump timelog error\n");

			all_pAd(i, pAd_list, pAd_num, pAd) {
				UCHAR j;

				/* send BAR to all STAs */
				ba_refresh_bar_all(pAd);

				/* update Beacon frame if operating in AP mode. */
				for (j = 0; j < WDEV_NUM_MAX; j++) {
					wdev = pAd->wdev_list[j];
					if (wdev && HcIsRadioAcq(wdev) &&
						(wdev->wdev_type == WDEV_TYPE_AP)) {
						UpdateBeaconHandler(
							pAd,
							wdev,
							BCN_REASON(BCN_UPDATE_ALL_AP_RENEW));
					}
				}
			}
		} else {
			MTWF_DBG(NULL, DBG_CAT_HW, CATHW_SER, DBG_LVL_ERROR,
				"!!! SER CurStat=%u Event=0x%x !!!\n",
				ser_get_curr_state(main_pAd), event);
		}

		break;

	default:
		MTWF_DBG(NULL, DBG_CAT_HW, CATHW_SER, DBG_LVL_ERROR,
			"!!! SER CurStat=%u Event=0x%x !!!\n",
			ser_get_curr_state(main_pAd), event);
		break;
	}

	return;
}

void bellwether_ser_0_5_v1(struct ser_cb_t *ser_cb)
{
	UINT32 chip_id = ser_cb->chip_id;
	UINT32 hw_id = ser_cb->hw_id;
	UINT8 level = ser_cb->ser_level;
	UINT8 state = ser_cb->ser_state;
	UINT32 event = ser_cb->ser_event;
#ifdef WF_RESET_SUPPORT
	struct _RTMP_ADAPTER *pAd_list[MAX_NUM_OF_INF] = {NULL};
	struct _RTMP_ADAPTER *pAd;
	int pAd_num;
#define MT_WM_WDT_INT WF_WFDMA_HOST_DMA0_MCU2HOST_SW_INT_STA_mcu2host_wmcpu_wdt_MASK
#define MT_WA_WDT_INT WF_WFDMA_HOST_DMA0_MCU2HOST_SW_INT_STA_mcu2host_wacpu_wdt_MASK

	pAd_num = ser_get_all_pAd_by_chipid_hwid(pAd_list, chip_id, hw_id);
	pAd = pAd_list[0];

	if (pAd_num == 0 || pAd == NULL) {
		MTWF_DBG(NULL, DBG_CAT_HW, CATHW_SER, DBG_LVL_ERROR,
			"pAd_num is 0 or pAd is NULL\n");
		return;
	}

	if (ser_cb->ser_event & MT_WM_WDT_INT)
		PD_SET_WF_RESET_WM_COUNT(pAd->physical_dev, PD_GET_WF_RESET_WM_COUNT(pAd->physical_dev) + 1);

	if (ser_cb->ser_event & MT_WA_WDT_INT)
		PD_SET_WF_RESET_WA_COUNT(pAd->physical_dev, PD_GET_WF_RESET_WA_COUNT(pAd->physical_dev) + 1);

	wf_reset_func(pAd->physical_dev);
#endif

	MTWF_DBG(NULL, DBG_CAT_HW, CATHW_SER, DBG_LVL_WARN,
		"chip_id=0x%x,hw_id=%d,level=%d,state=%d,event=0x%x\n",
		chip_id, hw_id, level, state, event);

}

void bellwether_ser_0_0_v1(struct ser_cb_t *ser_cb)
{
	UINT32 chip_id = ser_cb->chip_id;
	UINT32 hw_id = ser_cb->hw_id;
	UINT8 level = ser_cb->ser_level;
	UINT8 state = ser_cb->ser_state;
	UINT32 event = ser_cb->ser_event;

	MTWF_DBG(NULL, DBG_CAT_HW, CATHW_SER, DBG_LVL_WARN,
		"chip_id=0x%x,hw_id=%d,level=%d,state=%d,event=0x%x\n",
		chip_id, hw_id, level, state, event);

}

void bellwether_dump_ser_stat(struct _RTMP_ADAPTER *ad, UINT8 dump_all)
{
	UINT32 reg_tmp_val = 0;
	struct ser_dump_list {
		char *name;
		UINT32 reg;
	} cr_list[] = {
		{"SER_STATUS       ", WF_SW_DEF_CR_SER_STATUS_ADDR},
		{"SER_PLE_ERR      ", WF_SW_DEF_CR_PLE_STATUS_ADDR},
		{"SER_PLE_ERR_1    ", WF_SW_DEF_CR_PLE1_STATUS_ADDR},
		{"SER_PLE_ERR_AMSDU", WF_SW_DEF_CR_PLE_AMSDU_STATUS_ADDR},
		{"SER_PSE_ERR      ", WF_SW_DEF_CR_PSE_STATUS_ADDR},
		{"SER_PSE_ERR_1    ", WF_SW_DEF_CR_PSE1_STATUS_ADDR},
		{"SER_LMAC_WISR6_B0", WF_SW_DEF_CR_LAMC_WISR6_BN0_STATUS_ADDR},
		{"SER_LMAC_WISR6_B1", WF_SW_DEF_CR_LAMC_WISR6_BN1_STATUS_ADDR},
		{"SER_LMAC_WISR6_B2", WF_SW_DEF_CR_LAMC_WISR6_BN2_STATUS_ADDR},
		{"SER_LMAC_WISR7_B0", WF_SW_DEF_CR_LAMC_WISR7_BN0_STATUS_ADDR},
		{"SER_LMAC_WISR7_B1", WF_SW_DEF_CR_LAMC_WISR7_BN1_STATUS_ADDR},
		{"SER_LMAC_WISR7_B2", WF_SW_DEF_CR_LAMC_WISR7_BN2_STATUS_ADDR},
		{"SER_WFDMA_ERR    ", WF_SW_DEF_CR_WFDMA_STATUS_ADDR},
	};
	UINT i, cr_list_num = ARRAY_SIZE(cr_list);
	UINT war_list_num;
	UINT32 en_mask = 0;
	struct war_dump_list {
		char *name;
		UINT32 en_bit;
		UINT32 reg;
	} *war_list;


	struct war_dump_list war_list_e1[] = {
	};

	struct war_dump_list war_list_e2[] = {
	};

	if (MTK_REV_ET(ad, BELLWETHER, BELLWETHERE1)) {
		war_list = war_list_e1;
		war_list_num = ARRAY_SIZE(war_list_e1);
	} else {
		war_list = war_list_e2;
		war_list_num = ARRAY_SIZE(war_list_e2);
	}

	for (i = 0; i < cr_list_num; i++) {
		MAC_IO_READ32(ad->hdev_ctrl, cr_list[i].reg, &reg_tmp_val);
		if (reg_tmp_val != 0) {
			MTWF_DBG(NULL, DBG_CAT_HW, CATHW_SER, DBG_LVL_WARN,
				",::E  R , %s = 0x%08X\n",
				cr_list[i].name, reg_tmp_val);
		}
	}

	if (dump_all) {
		/* dump HWITS workaround info */
		/* TODO: WF_SW_DEF_HWITS_WAR_EN_MASK is not defined yet */
		/*MAC_IO_READ32(ad->hdev_ctrl, WF_SW_DEF_HWITS_WAR_EN_MASK, &en_mask);*/
		MTWF_DBG(NULL, DBG_CAT_HW, CATHW_SER, DBG_LVL_WARN,
				",::E  R , HWITS_WAR_EN_MASK = 0x%08X\n",
				en_mask);


		for (i = 0; i < war_list_num; i++) {
			MAC_IO_READ32(ad->hdev_ctrl, war_list[i].reg, &reg_tmp_val);
				MTWF_DBG(NULL, DBG_CAT_HW, CATHW_SER, DBG_LVL_WARN,
					"\t(WAR_%s) %s=%d\n",
					(en_mask & war_list[i].en_bit) ? "ENA" : "DIS",
					war_list[i].name, reg_tmp_val);
		}
	}
}

#endif /* ERR_RECOVERY */
#endif /* BELLWETHER */

