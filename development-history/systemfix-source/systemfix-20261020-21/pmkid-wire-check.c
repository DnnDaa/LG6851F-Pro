#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <assert.h>
#include <stdio.h>
#include <netlink/msg.h>
#include <netlink/attr.h>
typedef uint8_t u8;
typedef uint8_t UCHAR;
#define ETH_ALEN 6
#define PMKID_LEN 16
#define LEN_PMKID 16
#define os_memset memset
#define os_memcpy memcpy

struct wpa_pmkid_entry {
	u8 bssid[ETH_ALEN];
	u8 sta[ETH_ALEN]; /*MLD addr or link_addr*/
	u8 link_addr[ETH_ALEN];
	u8 pmkid[PMKID_LEN];
	u8 AddRemove;   /*1- ADD, 0- Remove*/
	int akmp;      /* Current native driver ABI: offset36, total40 bytes. */
};
typedef struct __RT_CMD_AP_IOCTL_UPDATE_PMKID {
	UCHAR bssid[ETH_ALEN]; /* AP's MLD addr */
	UCHAR sta[ETH_ALEN]; /* STA's MLD addr */
	UCHAR link_addr[ETH_ALEN]; /* Add: STA's link addr, Remove: AP's link addr */
	UCHAR pmkid[LEN_PMKID];
	UCHAR AddRemove;	/*1- Add, 0- Remove*/
	int akmp;		/* AKM protocol */
} RT_CMD_AP_IOCTL_UPDATE_PMKID;


#include "lg6851f_pmkid_abi.h"
_Static_assert(sizeof(RT_CMD_AP_IOCTL_UPDATE_PMKID)==40,"actual driver layout");
#define CHECK_FIELD(f) _Static_assert(offsetof(struct wpa_pmkid_entry,f)==offsetof(RT_CMD_AP_IOCTL_UPDATE_PMKID,f),"driver field offset: " #f)
CHECK_FIELD(bssid);CHECK_FIELD(sta);CHECK_FIELD(link_addr);CHECK_FIELD(pmkid);CHECK_FIELD(AddRemove);CHECK_FIELD(akmp);
int main(void) {
 u8 ap_mld[6]={0x02,1,2,3,4,5},sta_mld[6]={0x06,7,8,9,10,11};
 u8 sta_link[6]={0x0a,12,13,14,15,16},ap_link[6]={0x0e,17,18,19,20,21};
 u8 pmkid[16];int i,add,checks=0;
 for(i=0;i<16;i++)pmkid[i]=(u8)(0xa0+i);
 for(add=0;add<=1;add++) {
  struct wpa_pmkid_entry wire;
  RT_CMD_AP_IOCTL_UPDATE_PMKID received;
  struct nl_msg *msg=nlmsg_alloc();struct nlattr *attr;
  assert(msg && nlmsg_put(msg,0,0,0,0,0));
  memset(&wire,0xcc,sizeof(wire));
  lg6851f_fill_pmkid(&wire,(u8)add,ap_mld,sta_mld,add?sta_link:ap_link,pmkid,0x12345678);
  assert(nla_put(msg,1,sizeof(wire),&wire)==0);attr=nlmsg_data(nlmsg_hdr(msg));
  assert(nla_len(attr)==40);checks++;
  memcpy(&received,nla_data(attr),sizeof(received));
  assert(!memcmp(received.bssid,ap_mld,6));checks++;
  assert(!memcmp(received.sta,sta_mld,6));checks++;
  assert(!memcmp(received.link_addr,add?sta_link:ap_link,6));checks++;
  assert(!memcmp(received.pmkid,pmkid,16));checks++;
  assert(received.AddRemove==add);checks++;
  assert(received.akmp==0x12345678);checks++;
  assert(((u8*)&wire)[35]==0);checks++;
  nlmsg_free(msg);
 }
 printf("{\"wire_assertions\":%d,\"native_payload_bytes\":40,\"akmp_offset\":36,\"driver_fields_match\":true,\"passed\":true}\n",checks);
 return 0;
}
