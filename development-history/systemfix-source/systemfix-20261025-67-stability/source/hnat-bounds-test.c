#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
typedef uint16_t u16;
#define __packed __attribute__((packed))
#define TAG_PROTECT 0x6789
#define CB_OFFSET 40
#define FOE_INFO_LEN 14
struct pdma_rx_desc_info4 {u16 MAGIC_TAG_PROTECT;unsigned char rest[12];} __packed;
struct head_rx_descinfo4 {unsigned char rest[12];u16 MAGIC_TAG_PROTECT;} __packed;
struct sk_buff {unsigned char prefix[40],cb[48];uintptr_t dst;unsigned char *head,*end;size_t room,tail;};
#define skb_headroom(s) ((s)->room)
#define skb_tailroom(s) ((s)->tail)
#define skb_end_pointer(s) ((s)->end)
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

#define IS_SPACE_AVAILABLE_HEAD(skb)  \
	((((skb_headroom(skb) >= FOE_INFO_LEN) ? 1 : 0)))
#define IS_SPACE_AVAILABLE_TAIL(skb)  \
	(((skb_tailroom(skb) >= FOE_INFO_LEN) ? 1 : 0))
#define IS_SPACE_AVAILABLE_CB(skb) (CB_OFFSET + sizeof(struct cb_rx_desc_info4) <= sizeof((skb)->cb))
#define FOE_TAG_PROTECT_HEAD(skb)  \
	(((struct head_rx_descinfo4 *)((skb)->head))->MAGIC_TAG_PROTECT)
#define FOE_TAG_PROTECT_TAIL(skb)  \
	(((struct pdma_rx_desc_info4 *)((long)((skb_end_pointer(skb)) - FOE_INFO_LEN)))->MAGIC_TAG_PROTECT)
#define FOE_TAG_PROTECT_CB0(skb)  \
	(((struct cb_rx_desc_info4 *)((skb)->cb + CB_OFFSET))->MAGIC_TAG_PROTECT0)
#define FOE_TAG_PROTECT_CB1(skb)  \
	(((struct cb_rx_desc_info4 *)((skb)->cb + CB_OFFSET))->MAGIC_TAG_PROTECT1)
#define IS_MAGIC_TAG_PROTECT_VALID_HEAD(skb)  \
	(IS_SPACE_AVAILABLE_HEAD(skb) && FOE_TAG_PROTECT_HEAD(skb) == TAG_PROTECT)
#define IS_MAGIC_TAG_PROTECT_VALID_TAIL(skb)  \
	(IS_SPACE_AVAILABLE_TAIL(skb) && FOE_TAG_PROTECT_TAIL(skb) == TAG_PROTECT)
#define IS_MAGIC_TAG_PROTECT_VALID_CB(skb)  \
	(IS_SPACE_AVAILABLE_CB(skb) && (FOE_TAG_PROTECT_CB0(skb) == TAG_PROTECT) && \
	(FOE_TAG_PROTECT_CB0(skb) == FOE_TAG_PROTECT_CB1(skb)))

int main(void){
 assert(sizeof(struct cb_rx_desc_info4)==14);assert(offsetof(struct sk_buff,cb)==40);assert(offsetof(struct sk_buff,dst)==88);
 struct sk_buff skb={0};struct cb_rx_desc_info4 metadata={0};metadata.MAGIC_TAG_PROTECT0=metadata.MAGIC_TAG_PROTECT1=TAG_PROTECT;
 /* Demonstrate original 14-byte-at-cb40 corruption inside the real layout. */
 skb.dst=0x1122334455667788ULL;memcpy(skb.cb+CB_OFFSET,&metadata,sizeof(metadata));assert(skb.dst!=0x1122334455667788ULL);
 skb.dst=0x1122334455667788ULL;
 for(int i=0;i<10000;i++){
  assert(!IS_SPACE_AVAILABLE_CB(&skb));assert(!IS_MAGIC_TAG_PROTECT_VALID_CB(&skb));assert(skb.dst==0x1122334455667788ULL);
  skb.head=malloc(1);skb.end=skb.head+1;skb.room=skb.tail=0;
  assert(!IS_MAGIC_TAG_PROTECT_VALID_HEAD(&skb));assert(!IS_MAGIC_TAG_PROTECT_VALID_TAIL(&skb));free(skb.head);
  skb.head=calloc(1,64);skb.end=skb.head+64;skb.room=skb.tail=14;
  FOE_TAG_PROTECT_HEAD(&skb)=TAG_PROTECT;FOE_TAG_PROTECT_TAIL(&skb)=TAG_PROTECT;
  assert(IS_MAGIC_TAG_PROTECT_VALID_HEAD(&skb));assert(IS_MAGIC_TAG_PROTECT_VALID_TAIL(&skb));free(skb.head);
 }
 puts("HNAT_BOUNDS_CHECKS=70005");return 0;
}
