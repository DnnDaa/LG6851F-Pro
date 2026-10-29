from pathlib import Path
import sys
p=Path(sys.argv[1]); s=p.read_text(); start=s.index('INT ap_ieee_802_11_data_rx('); end=s.index('\nstruct wifi_dev_ops ap_wdev_ops',start+4); head=s[:start]; section=s[start:end]; tail=s[end:]
a='#endif /* TXRX_STAT_SUPPORT */\n\t\treturn FALSE;\n\t}\n\n\t/* 2. QOS */'
b='#endif /* TXRX_STAT_SUPPORT */\n\t\t/* An authenticated STA may send a complete 802.11 Null frame with\n\t\t * no data. Consume it normally; do not report a data RX failure. */\n\t\tif (pFmeCtrl->Type == FC_TYPE_DATA &&\n\t\t    pFmeCtrl->SubType == SUBTYPE_DATA_NULL &&\n\t\t    pRxBlk->DataSize == hdr_len) {\n\t\t\tRELEASE_NDIS_PACKET(pAd, pRxBlk->pRxPacket, NDIS_STATUS_SUCCESS);\n\t\t\treturn TRUE;\n\t\t}\n\t\treturn FALSE;\n\t}\n\n\t/* 2. QOS */'
assert section.count(a)==1, section.count(a); section=section.replace(a,b)
a='#endif /* TXRX_STAT_SUPPORT */\n\t\t\treturn FALSE;\n\t\t}\n\t}\n\n\tpRxBlk->UserPriority = UserPriority;'
b='#endif /* TXRX_STAT_SUPPORT */\n\t\t\t/* A complete QoS Null contains the two-byte QoS header only. */\n\t\t\tif (pFmeCtrl->Type == FC_TYPE_DATA &&\n\t\t\t    pFmeCtrl->SubType == SUBTYPE_QOS_NULL &&\n\t\t\t    pRxBlk->DataSize == 2) {\n\t\t\t\tRELEASE_NDIS_PACKET(pAd, pRxBlk->pRxPacket, NDIS_STATUS_SUCCESS);\n\t\t\t\treturn TRUE;\n\t\t\t}\n\t\t\treturn FALSE;\n\t\t}\n\t}\n\n\tpRxBlk->UserPriority = UserPriority;'
assert section.count(a)==1, section.count(a);section=section.replace(a,b)
p.write_text(head+section+tail)
print('Patched two exact-length null branches')
