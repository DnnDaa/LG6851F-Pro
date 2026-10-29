VOID PeerProtectEHTAction(PRTMP_ADAPTER pAd, MLME_QUEUE_ELEM *Elem)
{
	struct eht_prot_action_frame *eht_prot_act = (struct eht_prot_action_frame *)Elem->Msg;
	UINT8 Action = eht_prot_act->prot_eht_action, *pos;
	struct wifi_dev *wdev = Elem->wdev;
	MAC_TABLE_ENTRY *pEntry = NULL;
	struct mld_entry_t *mld_entry = NULL;
	uint16_t mld_sta_idx, mld_link_num;

	if (!VALID_UCAST_ENTRY_WCID(pAd, Elem->Wcid) || !wdev)
		return;

	switch (Action) {
	case EHT_PROT_ACT_T2LM_REQUEST:
	case EHT_PROT_ACT_T2LM_RESPONSE:
	case EHT_PROT_ACT_T2LM_TEARDOWN:
		nt2lm_peer_t2lm_req_action(pAd, Elem);
		break;
#ifdef EPCS_SUPPORT
	case EHT_PROT_ACT_EPCS_REQUEST:
	case EHT_PROT_ACT_EPCS_RESPONSE:
	case EHT_PROT_ACT_EPCS_TEARDOWN:
		MTWF_DBG(pAd, DBG_CAT_PROTO, CATPROTO_EHT, DBG_LVL_NOTICE,
			"received EPCS Action(%d) Frame.\n", Action);

		pEntry = entry_get(pAd, Elem->Wcid);
		if (!pEntry) {
			MTWF_DBG(pAd, DBG_CAT_PROTO, CATPROTO_EHT, DBG_LVL_ERROR,
				"can't find pEntry\n");
			return;
		}
#ifdef DOT11_EHT_BE
		MTWF_DBG(pAd, DBG_CAT_PROTO, CATPROTO_EHT, DBG_LVL_NOTICE,
			"pEntry->Addr:%pM, &Elem->Msg[10]:%pM, pEntry->mlo.mld_addr:%pM\n",
			pEntry->Addr, &Elem->Msg[10], pEntry->mlo.mld_addr);
		if (pEntry->mlo.mlo_en && !MAC_ADDR_EQUAL(pEntry->Addr, pEntry->mlo.mld_addr)) {
			MTWF_DBG(pAd, DBG_CAT_PROTO, CATPROTO_EHT, DBG_LVL_NOTICE,
				"Replace mac to mld mac for action frame.\n");
			NdisCopyMemory(&Elem->Msg[10], pEntry->mlo.mld_addr, MAC_ADDR_LEN);
		}
#endif

		receive_qos_action_frame(pAd, Elem->wdev, &Elem->Msg[10],
			Elem->Channel, Elem->Msg, Elem->MsgLen);
		break;
#endif
	case EHT_PROT_ACT_EML_OP_NOTIF:
		{
			UINT32 eml_mode_bmap = 0;
			struct mld_eml_op eml_op = {0};

			pEntry = entry_get(pAd, Elem->Wcid);
			if (!pEntry) {
				MTWF_DBG(pAd, DBG_CAT_PROTO, CATPROTO_EHT, DBG_LVL_ERROR,
					"can't find pEntry\n");
				return;
			}
			mt_rcu_read_lock();
			if (pEntry->mlo.mlo_en)
				mld_entry = rcu_dereference(pEntry->mld_entry);
			if (!mld_entry) {
				MTWF_DBG(pAd, DBG_CAT_PROTO, CATPROTO_EHT, DBG_LVL_ERROR,
					"[EML Op Notif] mld_entry=NULL\n");
				mt_rcu_read_unlock();
				break;
			}

			mld_link_num = mld_entry->link_num;
			mld_sta_idx = mld_entry->mld_sta_idx;
			mt_rcu_read_unlock();

			MTWF_DBG(pAd, DBG_CAT_PROTO, CATPROTO_EHT, DBG_LVL_INFO,
				"[EML Op Notif] MsgLen=%ld, wcid=%d, wdev=%s, assoc_link_num=%d\n",
				Elem->MsgLen, Elem->Wcid, wdev->if_dev->name,
				mld_link_num);

			pos = eht_prot_act->data;

			NdisMoveMemory(&eml_op.dialog_token, pos, sizeof(eml_op.dialog_token));
			pos += sizeof(eml_op.dialog_token);

			NdisMoveMemory(&eml_mode_bmap, pos, sizeof(eml_mode_bmap));

			eml_op.mld_sta_idx = mld_sta_idx;
			eml_op.emlsr_en = GET_DOT11BE_EML_CTRL_EMLSR_MODE(eml_mode_bmap);
			eml_op.emlmr_en = GET_DOT11BE_EML_CTRL_EMLMR_MODE(eml_mode_bmap);
			eml_op.emlsr_param_upd_ctrl = GET_DOT11BE_EML_CTRL_EMLSR_PARAM_UPD_CTRL(eml_mode_bmap);
			if (eml_op.emlsr_en || eml_op.emlmr_en) { /* D3.0 */
				eml_op.eml_link_bmap = GET_DOT11BE_EML_CTRL_EMLSR_LINK_BMAP(eml_mode_bmap);
				eml_op.eml_link_bmap = cpu_to_le16(eml_op.eml_link_bmap);
			}
			eml_op.priv_ptr = (void *)pEntry;

			MTWF_DBG(pAd, DBG_CAT_PROTO, CATPROTO_EHT, DBG_LVL_ERROR,
				"[EML Op Notif] mld_sta_idx=%d, EMLSR_en=%d, EMLSR_bmap=0x%x, token=%d\n",
				eml_op.mld_sta_idx, eml_op.emlsr_en, eml_op.eml_link_bmap,
				eml_op.dialog_token);

			if (wdev->wdev_type == WDEV_TYPE_AP)
				bss_mngr_mld_eml_op_update(wdev, &eml_op);
		}
		break;

	default:
		break;
	}
}