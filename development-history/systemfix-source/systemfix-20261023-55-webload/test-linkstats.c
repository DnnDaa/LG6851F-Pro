#define _GNU_SOURCE
#include <stdio.h>
#include <assert.h>
#include "lg6851f-linkstats.h"
static unsigned int checks;
#define CHECK(x) do { if(!(x)){fprintf(stderr,"failed line %d\n",__LINE__);exit(2);}checks++; } while(0)
static size_t fixture(unsigned char *buf, uint64_t bytes)
{
 struct nlmsghdr *n=(void*)buf;
 memset(buf,0,1024);n->nlmsg_len=NLMSG_LENGTH(sizeof(struct ifinfomsg));n->nlmsg_type=RTM_NEWLINK;n->nlmsg_seq=77;
 struct ifinfomsg *info=NLMSG_DATA(n);info->ifi_index=42;
 struct rtattr *a=(void*)(buf+NLMSG_ALIGN(n->nlmsg_len));a->rta_type=IFLA_IFNAME;a->rta_len=RTA_LENGTH(7);memcpy(RTA_DATA(a),"ccmni2",7);n->nlmsg_len=NLMSG_ALIGN(n->nlmsg_len)+RTA_ALIGN(a->rta_len);
 a=(void*)(buf+NLMSG_ALIGN(n->nlmsg_len));a->rta_type=IFLA_STATS64;a->rta_len=RTA_LENGTH(sizeof(struct rtnl_link_stats64));
 struct rtnl_link_stats64 s={.rx_bytes=bytes,.tx_bytes=bytes+9,.rx_packets=123,.tx_packets=456};memcpy(RTA_DATA(a),&s,sizeof(s));n->nlmsg_len=NLMSG_ALIGN(n->nlmsg_len)+RTA_ALIGN(a->rta_len);
 return NLMSG_ALIGN(n->nlmsg_len);
}
int main(void)
{
 unsigned char data[1024] __attribute__((aligned(8)));struct lg_linkstats_cache cache={0};size_t size=fixture(data,UINT64_C(0x100000011));
 CHECK(lg_linkstats_parse(&cache,data,size,77)==0);CHECK(cache.count==1);CHECK(cache.entries[0].stats.rx_bytes==UINT64_C(0x100000011));CHECK(cache.entries[0].stats.tx_bytes==UINT64_C(0x10000001a));CHECK(!strcmp(cache.entries[0].name,"ccmni2"));CHECK(cache.entries[0].index==42);
 CHECK(lg_linkstats_parse(&cache,data,size,77)<0);cache.count=0;
 CHECK(lg_linkstats_parse(&cache,data,size,76)<0);
 ((struct nlmsghdr*)data)->nlmsg_flags=NLM_F_DUMP_INTR;CHECK(lg_linkstats_parse(&cache,data,size,77)<0);
 size=fixture(data,1);((struct nlmsghdr*)data)->nlmsg_len=NLMSG_HDRLEN;CHECK(lg_linkstats_parse(&cache,data,size,77)<0);
 size=fixture(data,1);CHECK(lg_linkstats_parse(&cache,data,size-1,77)<0);
 memset(data,0,sizeof(data));struct nlmsghdr *done=(void*)data;done->nlmsg_type=NLMSG_DONE;done->nlmsg_seq=77;done->nlmsg_len=NLMSG_LENGTH(sizeof(int));CHECK(lg_linkstats_parse(&cache,data,done->nlmsg_len,77)==1);
 int error=-EINTR;memcpy(NLMSG_DATA(done),&error,sizeof(error));CHECK(lg_linkstats_parse(&cache,data,done->nlmsg_len,77)<0);
 srand(55);
 for(unsigned i=0;i<5000;i++) {
  size=fixture(data,UINT64_C(0xffffffffffff0000)+i);cache.count=0;
  unsigned edits=1+rand()%4;
  for(unsigned e=0;e<edits;e++)data[rand()%size]=(unsigned char)rand();
  int result=lg_linkstats_parse(&cache,data,rand()%(size+1),77);
  CHECK(result<=1);CHECK(cache.count<=LG_LINKSTATS_MAX);
 }
 /* Exercise the real read-only GETLINK transport and stable lookup. */
 CHECK(lg_linkstats_refresh()==0);CHECK(lg_linkstats_cache.count>0);
 struct rtnl_link_stats64 real;CHECK(lg_linkstats_get("lo",&real)==0);CHECK(lg_linkstats_get("missing-nic",&real)<0);
 printf("LINKSTATS_CHECKS=%u\n",checks);return 0;
}
