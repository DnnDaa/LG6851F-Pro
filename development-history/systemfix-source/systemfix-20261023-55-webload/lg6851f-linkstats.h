/* SPDX-License-Identifier: GPL-2.0-only */
/* Bounded, cache-only reuse of one read-only RTM_GETLINK statistics dump. */
#ifndef LG6851F_LINKSTATS_H
#define LG6851F_LINKSTATS_H
#include <errno.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <stddef.h>
#include <unistd.h>
#include <time.h>
#include <poll.h>
#include <sys/socket.h>
#include <net/if.h>
#include <linux/netlink.h>
#include <linux/rtnetlink.h>
#include <linux/if_link.h>

#define LG_LINKSTATS_MAX 256
struct lg_linkstats_entry {
 char name[IF_NAMESIZE];
 unsigned int index;
 struct rtnl_link_stats64 stats;
};
struct lg_linkstats_cache {
 struct lg_linkstats_entry entries[LG_LINKSTATS_MAX];
 size_t count;
 int64_t stamp_ms;
 bool valid;
};
static struct lg_linkstats_cache lg_linkstats_cache;

static int64_t lg_linkstats_now(void)
{
 struct timespec ts;
 if (clock_gettime(CLOCK_MONOTONIC, &ts)) return -1;
 return (int64_t)ts.tv_sec * 1000 + ts.tv_nsec / 1000000;
}

/* Called on a complete received datagram; rejects partial or inconsistent data. */
static int lg_linkstats_parse(struct lg_linkstats_cache *cache,
 const void *data, size_t bytes, uint32_t seq)
{
 const struct nlmsghdr *nh = data;
 int remain;
 if (bytes > INT32_MAX) return -1;
 remain = (int)bytes;
 for (; NLMSG_OK(nh, remain); nh = NLMSG_NEXT(nh, remain)) {
  const struct ifinfomsg *ifi;
  const struct rtattr *attr;
  const char *name = NULL;
  const void *stats = NULL;
  size_t stats_len = 0;
  int len;
  if (nh->nlmsg_seq != seq || (nh->nlmsg_flags & NLM_F_DUMP_INTR)) return -1;
  if (nh->nlmsg_type == NLMSG_DONE) {
   if (remain != (int)NLMSG_ALIGN(nh->nlmsg_len)) return -1;
   if (NLMSG_PAYLOAD(nh, 0) >= sizeof(int) && *(const int *)NLMSG_DATA(nh)) return -1;
   return 1;
  }
  if (nh->nlmsg_type == NLMSG_ERROR) return -1;
  if (nh->nlmsg_type != RTM_NEWLINK) continue;
  if (nh->nlmsg_len < NLMSG_LENGTH(sizeof(*ifi))) return -1;
  ifi = NLMSG_DATA(nh);
  if (ifi->ifi_index <= 0) return -1;
  len = IFLA_PAYLOAD(nh);
  for (attr = IFLA_RTA(ifi); RTA_OK(attr, len); attr = RTA_NEXT(attr, len)) {
   if (attr->rta_type == IFLA_IFNAME) {
    size_t n = RTA_PAYLOAD(attr);
    if (name || !n || n > IF_NAMESIZE || !memchr(RTA_DATA(attr), 0, n)) return -1;
    name = RTA_DATA(attr);
    if (!name[0]) return -1;
   } else if (attr->rta_type == IFLA_STATS64) {
    if (stats) return -1;
    stats = RTA_DATA(attr); stats_len = RTA_PAYLOAD(attr);
   }
  }
  if (len) return -1;
  if (!name || !stats) continue;
  if (stats_len < offsetof(struct rtnl_link_stats64, tx_compressed) + sizeof(uint64_t)) return -1;
  if (cache->count >= LG_LINKSTATS_MAX) return -1;
  for (size_t i = 0; i < cache->count; i++)
   if (cache->entries[i].index == (unsigned)ifi->ifi_index || !strcmp(cache->entries[i].name, name)) return -1;
  struct lg_linkstats_entry *entry = &cache->entries[cache->count++];
  memset(entry, 0, sizeof(*entry));
  memcpy(entry->name, name, strlen(name) + 1);
  entry->index = ifi->ifi_index;
  memcpy(&entry->stats, stats, stats_len < sizeof(entry->stats) ? stats_len : sizeof(entry->stats));
 }
 return remain ? -1 : 0;
}

static int lg_linkstats_refresh(void)
{
 struct {struct nlmsghdr nh; struct ifinfomsg ifi;} req = {
  .nh = {.nlmsg_len=NLMSG_LENGTH(sizeof(struct ifinfomsg)),
   .nlmsg_type=RTM_GETLINK,.nlmsg_flags=NLM_F_REQUEST|NLM_F_DUMP,.nlmsg_seq=1},
  .ifi = {.ifi_family=AF_UNSPEC}
 };
 struct sockaddr_nl addr = {.nl_family=AF_NETLINK};
 struct lg_linkstats_cache *next;
 int64_t start = lg_linkstats_now();
 int fd = -1, result = -1;
 unsigned char *buffer;
 if (start < 0) return -1;
 next = calloc(1, sizeof(*next)); buffer = malloc(65536);
 if (!next || !buffer) goto out;
 fd = socket(AF_NETLINK, SOCK_RAW|SOCK_CLOEXEC|SOCK_NONBLOCK, NETLINK_ROUTE);
 if (fd < 0 || bind(fd, (struct sockaddr *)&addr, sizeof(addr))) goto out;
 if (sendto(fd, &req, sizeof(req), 0, (struct sockaddr *)&addr, sizeof(addr)) != sizeof(req)) goto out;
 for (;;) {
  struct pollfd pfd = {.fd=fd,.events=POLLIN};
  struct sockaddr_nl peer = {0};
  struct iovec iov = {.iov_base=buffer,.iov_len=65536};
  struct msghdr msg = {.msg_name=&peer,.msg_namelen=sizeof(peer),.msg_iov=&iov,.msg_iovlen=1};
  int64_t now = lg_linkstats_now();
  ssize_t n;
  int rc;
  if (now < start || now-start >= 250) goto out;
  rc = poll(&pfd, 1, 250-(int)(now-start));
  if (rc < 0 && errno == EINTR) continue;
  if (rc <= 0 || !(pfd.revents & POLLIN)) goto out;
  n = recvmsg(fd, &msg, 0);
  if (n < 0 && (errno == EINTR || errno == EAGAIN)) continue;
  if (n <= 0 || msg.msg_flags & MSG_TRUNC || peer.nl_pid != 0) goto out;
  rc = lg_linkstats_parse(next, buffer, (size_t)n, req.nh.nlmsg_seq);
  if (rc < 0) goto out;
  if (rc > 0) {
   next->stamp_ms=lg_linkstats_now();next->valid=true;
   lg_linkstats_cache=*next; result=0;goto out;
  }
 }
out:
 if (fd >= 0) close(fd);
 free(buffer);free(next);
 return result;
}

/* Same name AND ifindex required: do not reuse a deleted/recreated interface. */
static int lg_linkstats_get(const char *name, struct rtnl_link_stats64 *out)
{
 int64_t now=lg_linkstats_now();
 unsigned int index;
 if (!name || strlen(name) >= IF_NAMESIZE || !out || now < 0) return -1;
 if (!lg_linkstats_cache.valid || now < lg_linkstats_cache.stamp_ms || now-lg_linkstats_cache.stamp_ms >= 200)
  if (lg_linkstats_refresh()) return -1;
 index=if_nametoindex(name);
 if (!index) return -1;
 for (size_t i=0;i<lg_linkstats_cache.count;i++) {
  struct lg_linkstats_entry *entry=&lg_linkstats_cache.entries[i];
  if (entry->index==index && !strcmp(entry->name,name)) {*out=entry->stats;return 0;}
 }
 return -1;
}
#endif
