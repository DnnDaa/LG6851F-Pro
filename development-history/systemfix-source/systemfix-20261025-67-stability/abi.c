#include <linux/module.h>
#include <linux/skbuff.h>
typedef u16 uint16_t;typedef u32 uint32_t;
struct cb_rx_desc_info4 {
	u16 MAGIC_TAG_PROTECT0;
	uint32_t foe_entry_num:15;
	uint32_t CRSN:5;
	uint32_t SPORT:4;
	uint32_t ALG:1;
	uint32_t rsv:7;
	uint16_t IF:8;
	uint16_t WDMAID:2;
	uint16_t RXID:2;
	uint16_t WCID:10;
	uint16_t BSSID:6;
	uint16_t rsv1:2;
	uint16_t minfo:1;
	uint16_t ntype:3;
	uint16_t chid:8;
	uint16_t IF_IDX:6;
#if defined(CONFIG_RA_HW_NAT_PPTP_L2TP)
	u16 SOURCE;
	u16 DEST;
#endif
	u16 MAGIC_TAG_PROTECT1;
} __packed;
const unsigned long lg_abi[] = {sizeof(struct sk_buff),offsetof(struct sk_buff,cb),sizeof(((struct sk_buff *)0)->cb),offsetof(struct sk_buff,_skb_refdst),sizeof(struct cb_rx_desc_info4),offsetof(struct cb_rx_desc_info4,MAGIC_TAG_PROTECT1),40};
MODULE_LICENSE("GPL");
