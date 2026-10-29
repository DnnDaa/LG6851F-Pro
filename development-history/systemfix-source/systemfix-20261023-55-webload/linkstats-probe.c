/* Read-only probe: only RTM_GETLINK requests; no service registration. */
#define _GNU_SOURCE
#include <stdio.h>
#include <inttypes.h>
#include "lg6851f-linkstats.h"
int main(void)
{
 int64_t start=lg_linkstats_now();
 if(lg_linkstats_refresh()) {perror("GETLINK");return 1;}
 printf("elapsed_ms=%" PRId64 " count=%zu\n",lg_linkstats_now()-start,lg_linkstats_cache.count);
 for(size_t i=0;i<lg_linkstats_cache.count;i++) {
  struct lg_linkstats_entry *entry=&lg_linkstats_cache.entries[i];
  if(strncmp(entry->name,"ccmni",5) && strcmp(entry->name,"br-lan") && strncmp(entry->name,"eth",3))continue;
  struct rtnl_link_stats64 copied;
  if(lg_linkstats_get(entry->name,&copied))return 2;
  printf("%s rx=%" PRIu64 " tx=%" PRIu64 " rxp=%" PRIu64 " txp=%" PRIu64 "\n",entry->name,(uint64_t)copied.rx_bytes,(uint64_t)copied.tx_bytes,(uint64_t)copied.rx_packets,(uint64_t)copied.tx_packets);
 }
 return 0;
}
