#include "rt_config.h"
/* EML Control is one octet. The link bitmap and parameter update are optional. */
static int lg6_decode_eml(const u8 *input, unsigned int len, struct mld_eml_op *op)
{
	u8 ctrl;
	unsigned int used = 2;
	if (!input || !op || len < 2)
		return -1;
	ctrl = input[1];
	if ((ctrl & 3) == 3)
		return -1;
	op->dialog_token = input[0];
	op->emlsr_en = ctrl & 1;
	op->emlmr_en = (ctrl >> 1) & 1;
	op->emlsr_param_upd_ctrl = (ctrl >> 2) & 1;
	op->eml_link_bmap = 0;
	if (ctrl & 3) {
		if (len < 4)
			return -1;
		op->eml_link_bmap = input[2] | ((u16)input[3] << 8);
		if (!op->eml_link_bmap)
			return -1;
		used += 2;
	}
	if (op->emlsr_param_upd_ctrl && len < used + 1)
		return -1;
	return 0;
}

VOID lg6_PeerProtectEHTAction(PRTMP_ADAPTER, MLME_QUEUE_ELEM *);
#define PeerProtectEHTAction lg6_PeerProtectEHTAction
VOID PeerProtectEHTAction(PRTMP_ADAPTER pAd, MLME_QUEUE_ELEM *Elem)
{
	struct eht_prot_action_frame *eht_prot_act;
	UINT8 Action;
	struct wifi_dev *wdev;
	MAC_TABLE_ENTRY *pEntry = NULL;
	struct mld_entry_t *mld_entry = NULL;
	uint16_t mld_sta_idx, mld_link_num;

	if (!Elem || !pAd)
		return;
	wdev = Elem->wdev;
	eht_prot_act = (struct eht_prot_action_frame *)Elem->Msg;
	if (Elem->MsgLen < offsetof(struct eht_prot_action_frame, data))
		return;
	Action = eht_prot_act->prot_eht_action;
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

			if (lg6_decode_eml(eht_prot_act->data,
				Elem->MsgLen - offsetof(struct eht_prot_action_frame, data), &eml_op))
				return;
			eml_op.mld_sta_idx = mld_sta_idx;
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
