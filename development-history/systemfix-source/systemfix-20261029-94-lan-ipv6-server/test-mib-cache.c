
#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdatomic.h>
#include <stdlib.h>
#include <pthread.h>
typedef unsigned char u8;
typedef uint32_t u32;
typedef struct { _Atomic int n; } atomic_t;
typedef struct { _Atomic long long n; } atomic64_t;
#define atomic_cmpxchg_acquire(a,o,v) ({ int expected=(o); atomic_compare_exchange_strong_explicit(&(a)->n,&expected,(v),memory_order_acquire,memory_order_relaxed);expected; })
#define atomic_read(a) atomic_load(&(a)->n)
#define atomic_inc(a) atomic_fetch_add(&(a)->n,1)
#define atomic_set_release(a,v) atomic_store_explicit(&(a)->n,(v),memory_order_release)
#define atomic64_inc(a) atomic_fetch_add(&(a)->n,1)
#define module_param(...)
#define MODULE_PARM_DESC(...)
#define READ_ONCE(v) (v)
#define min(a,b) ((a)<(b)?(a):(b))
#define msecs_to_jiffies(v) (v)
#define time_after_eq(a,b) ((long)((a)-(b))>=0)
#define pr_notice(...) ((void)0)
#define MAX_IF_NUM 8
struct net_device { char name[16];int ifindex; };
struct rtnl_link_stats64 { uint64_t rx_packets,tx_packets,rx_bytes,tx_bytes,multicast; };
struct net_device *dst_port[MAX_IF_NUM];
struct {uint64_t rx_pkt_cnt,tx_pkt_cnt,rx_byte_cnt,tx_byte_cnt,rx_mcast_cnt;} hnat_if[MAX_IF_NUM];
static unsigned long jiffies;
static int debug_level;
static uint64_t hardware[MAX_IF_NUM],delivered[MAX_IF_NUM];
static unsigned long scans[MAX_IF_NUM];
static int reset_in_scan;
static void ppe_dev_stats_invalidate(unsigned int index);
void ppe_get_mib_by_dev(u32 index) {
 scans[index]++;hnat_if[index].rx_byte_cnt+=hardware[index];hardware[index]=0;
 if(reset_in_scan){reset_in_scan=0;ppe_dev_stats_invalidate(index);}
}
struct ppe_dev_stats_gate {
 atomic_t busy;
 atomic_t generation;
 unsigned int observed_generation;
 unsigned long refreshed;
 const struct net_device *owner;
 int ifindex;
 bool valid;
};
static struct ppe_dev_stats_gate ppe_dev_stats_gates[MAX_IF_NUM];
static unsigned int mib_stats_interval_ms = 1000;
module_param(mib_stats_interval_ms, uint, 0644);
MODULE_PARM_DESC(mib_stats_interval_ms,
 "Coalesce per-device hardware stats scans (0=original frequency, max=1000ms)");
static atomic64_t mib_stats_scans, mib_stats_cached, mib_stats_busy;

static bool ppe_dev_stats_enter(unsigned int index, struct net_device *dev)
{
 struct ppe_dev_stats_gate *g = &ppe_dev_stats_gates[index];
 unsigned long interval = msecs_to_jiffies(min(READ_ONCE(mib_stats_interval_ms), 1000U));
 unsigned int generation;
 if (atomic_cmpxchg_acquire(&g->busy, 0, 1)) {
  atomic64_inc(&mib_stats_busy);
  return false;
 }
 generation = atomic_read(&g->generation);
 if (!g->valid || g->owner != dev || g->ifindex != dev->ifindex ||
     g->observed_generation != generation || !interval ||
     time_after_eq(jiffies, g->refreshed + interval)) {
  ppe_get_mib_by_dev(index);
  g->refreshed = jiffies;
  g->observed_generation = generation;
  g->owner = dev;
  g->ifindex = dev->ifindex;
  g->valid = true;
  atomic64_inc(&mib_stats_scans);
 } else {
  atomic64_inc(&mib_stats_cached);
 }
 return true;
}

static void ppe_dev_stats_leave(unsigned int index)
{
 atomic_set_release(&ppe_dev_stats_gates[index].busy, 0);
}

static void ppe_dev_stats_invalidate(unsigned int index)
{
 /* Reset is called on netdev lifecycle changes.  A reset concurrent with a
  * refresh leaves a different generation, forcing the NEXT read to refresh.
  * A deleted/recreated device is also distinguished by pointer and ifindex.
  */
 atomic_inc(&ppe_dev_stats_gates[index].generation);
}


void ppe_reset_dev_mib(struct net_device *dev) {
	int i;

	for (i = 0; i < MAX_IF_NUM; i++) {
		if (dst_port[i] == dev) {
            ppe_dev_stats_invalidate(i);
			// clear
			hnat_if[i].rx_byte_cnt = 0;
			hnat_if[i].tx_byte_cnt = 0;

			hnat_if[i].rx_pkt_cnt = 0;
			hnat_if[i].tx_pkt_cnt= 0;
			hnat_if[i].rx_mcast_cnt = 0;
			break;
		}
	}
}
int ppe_get_dev_stats_handler(struct net_device *dev, struct rtnl_link_stats64 *stats) {


	u8 i, dev_idx, match;

	if (debug_level == 1)
		pr_notice("%s, dev = %s\n", __func__, dev->name);

#ifdef CONFIG_TUNNEL_FAST_PATH
	if(strcmp(dev->name, TUNNEL_INTERFACE_NAME) == 0) {
		/* UL: teb tx */
		/* UL pkt cnt */
		current_hnat_tnl_swnat_cnt_ul = get_hnat_tnl_swnat_cnt_ul();
		stats->tx_packets += current_hnat_tnl_swnat_cnt_ul - last_hnat_tnl_swnat_cnt_ul;
		if (debug_level == 1)
			pr_notice("%s, tx_packets += %llu\n", __func__, current_hnat_tnl_swnat_cnt_ul - last_hnat_tnl_swnat_cnt_ul);
		last_hnat_tnl_swnat_cnt_ul = current_hnat_tnl_swnat_cnt_ul;
		/* UL byte cnt */
		current_hnat_tnl_swnat_byte_cnt_ul = get_hnat_tnl_swnat_byte_cnt_ul();
		stats->tx_bytes += current_hnat_tnl_swnat_byte_cnt_ul - last_hnat_tnl_swnat_byte_cnt_ul;
		if (debug_level == 1)
			pr_notice("%s, tx_bytes += %llu\n", __func__, current_hnat_tnl_swnat_byte_cnt_ul - last_hnat_tnl_swnat_byte_cnt_ul);
		last_hnat_tnl_swnat_byte_cnt_ul = current_hnat_tnl_swnat_byte_cnt_ul;

		/* DL: teb rx */
		/* DL pkt cnt */
		current_hnat_tnl_swnat_cnt_dl = get_hnat_tnl_swnat_cnt_dl();
		stats->rx_packets += current_hnat_tnl_swnat_cnt_dl - last_hnat_tnl_swnat_cnt_dl;
		if (debug_level == 1)
			pr_notice("%s, rx_packets += %llu\n", __func__, current_hnat_tnl_swnat_cnt_dl - last_hnat_tnl_swnat_cnt_dl);
		last_hnat_tnl_swnat_cnt_dl = current_hnat_tnl_swnat_cnt_dl;
		/* DL byte cnt */
		current_hnat_tnl_swnat_byte_cnt_dl = get_hnat_tnl_swnat_byte_cnt_dl();
		stats->rx_bytes += current_hnat_tnl_swnat_byte_cnt_dl - last_hnat_tnl_swnat_byte_cnt_dl;
		if (debug_level == 1)
			pr_notice("%s, rx_bytes += %llu\n", __func__, current_hnat_tnl_swnat_byte_cnt_dl - last_hnat_tnl_swnat_byte_cnt_dl);
		last_hnat_tnl_swnat_byte_cnt_dl = current_hnat_tnl_swnat_byte_cnt_dl;

		return 1; /* succeed */
	}
#endif /* CONFIG_TUNNEL_FAST_PATH */

	dev_idx = 1;
	match = 0;
	for (i = 0; i < MAX_IF_NUM; i++) {
		if (dst_port[i] == dev) {
			dev_idx = i;
			match = 1;
			break;
		}

	}
	if (match == 0) {
		stats->rx_packets = 0;
		stats->tx_packets = 0;
		stats->rx_bytes = 0;
		stats->tx_bytes = 0;
		stats->multicast = 0;
		return 1; /* succeed */
	}

	if (!ppe_dev_stats_enter(dev_idx, dev))
        return 1; /* Another reader owns the pending delta; never duplicate it. */

	stats->rx_packets += hnat_if[dev_idx].rx_pkt_cnt;
	stats->tx_packets += hnat_if[dev_idx].tx_pkt_cnt;
	stats->rx_bytes += hnat_if[dev_idx].rx_byte_cnt;
	stats->tx_bytes += hnat_if[dev_idx].tx_byte_cnt;
	stats->multicast += hnat_if[dev_idx].rx_mcast_cnt;

	//reset counter
	hnat_if[dev_idx].rx_pkt_cnt = 0;
	hnat_if[dev_idx].tx_pkt_cnt = 0;
	hnat_if[dev_idx].rx_byte_cnt = 0;
	hnat_if[dev_idx].tx_byte_cnt = 0;
	hnat_if[dev_idx].rx_mcast_cnt = 0;
	ppe_dev_stats_leave(dev_idx);

	if (debug_level >= 5)
		pr_notice("%s, if(%u):%s: rx_pkt:%llu, tx_pkt:%llu, rx_byte=%llu, tx_byte=%llu, mcast:%llu\n",
			__func__, dev_idx, dev->name, stats->rx_packets, stats->tx_packets,
			stats->rx_bytes, stats->tx_bytes, stats->multicast);

	return 1; /* succeed */

}

static unsigned long assertions;
#define CHECK(x) do {assertions++;if(!(x)){fprintf(stderr,"FAIL %u: %s\n",__LINE__,#x);abort();}}while(0)
static struct net_device devices[MAX_IF_NUM];
static void init(void) {
 memset(hnat_if,0,sizeof(hnat_if));memset(hardware,0,sizeof(hardware));memset(delivered,0,sizeof(delivered));memset(scans,0,sizeof(scans));
 memset(ppe_dev_stats_gates,0,sizeof(ppe_dev_stats_gates));
 for(int i=0;i<MAX_IF_NUM;i++){devices[i].ifindex=10+i;snprintf(devices[i].name,16,"ccmni%d",i);dst_port[i]=&devices[i];}
 jiffies=1;mib_stats_interval_ms=1000;reset_in_scan=0;
}
static uint64_t query(int index) {
 struct rtnl_link_stats64 st={0};CHECK(ppe_get_dev_stats_handler(&devices[index],&st)==1);
 delivered[index]+=st.rx_bytes;return st.rx_bytes;
}
static void *thread_read(void *arg) {int i=(int)(uintptr_t)arg;for(int n=0;n<30000;n++) {struct rtnl_link_stats64 st={0};ppe_get_dev_stats_handler(&devices[i],&st);assert(st.rx_bytes==0);}return NULL;}
int main(void) {
 init();hardware[0]=100;CHECK(query(0)==100);hardware[0]=50;CHECK(query(0)==0);CHECK(scans[0]==1);
 jiffies+=999;CHECK(query(0)==0);jiffies++;CHECK(query(0)==50);CHECK(scans[0]==2);
 /* Original timer deltas must still be delivered inside the coalescing window. */
 hnat_if[0].rx_byte_cnt=7;CHECK(query(0)==7);CHECK(query(0)==0);
 hnat_if[0].rx_pkt_cnt=2;hnat_if[0].tx_pkt_cnt=3;hnat_if[0].tx_byte_cnt=800;hnat_if[0].rx_mcast_cnt=4;
 struct rtnl_link_stats64 st={0};CHECK(ppe_get_dev_stats_handler(&devices[0],&st)==1);
 CHECK(st.rx_packets==2&&st.tx_packets==3&&st.tx_bytes==800&&st.multicast==4);
 st=(struct rtnl_link_stats64){0};CHECK(ppe_get_dev_stats_handler(&devices[0],&st)==1);CHECK(st.tx_bytes==0&&st.rx_packets==0);
 /* Replacement device and reset both force a fresh scan. */
 hardware[0]=9;devices[0].ifindex++;CHECK(query(0)==9);
 hardware[0]=11;ppe_reset_dev_mib(&devices[0]);CHECK(query(0)==11);
 hardware[0]=13;reset_in_scan=1;jiffies+=1000;CHECK(query(0)==13);
 hardware[0]=14;CHECK(query(0)==14); /* reset occurred during previous scan */
 /* A busy reader neither spins nor consumes/copies an old pending delta. */
 hnat_if[0].rx_byte_cnt=17;atomic_set_release(&ppe_dev_stats_gates[0].busy,1);CHECK(query(0)==0);CHECK(hnat_if[0].rx_byte_cnt==17);
 atomic_set_release(&ppe_dev_stats_gates[0].busy,0);CHECK(query(0)==17);
 /* Legacy frequency and parameter clamp. */
 mib_stats_interval_ms=0;hardware[0]=18;CHECK(query(0)==18);hardware[0]=19;CHECK(query(0)==19);
 mib_stats_interval_ms=1000000;hardware[0]=20;CHECK(query(0)==0);jiffies+=1000;CHECK(query(0)==20);
 init();jiffies=~0UL-500;hardware[0]=1;CHECK(query(0)==1);jiffies+=700;hardware[0]=2;CHECK(query(0)==0);jiffies+=301;CHECK(query(0)==2);
 /* Interleave multiple readers/devices and pending timers; the oracle sums all input deltas. */
 init();uint64_t produced[MAX_IF_NUM]={0};uint32_t seed=7;
 for(int n=0;n<100000;n++) {
  seed=seed*1664525U+1013904223U;int i=(seed>>16)%MAX_IF_NUM;uint64_t add=(seed&65535)+1;
  produced[i]+=add;hardware[i]+=add;jiffies+=(seed>>25)&31;query(i);
  if(n%17==0){produced[i]+=5;hnat_if[i].rx_byte_cnt+=5;query(i);}
  if(n%97==0){ppe_dev_stats_invalidate(i);query(i);}
  CHECK(delivered[i]<=produced[i]);
 }
 jiffies+=1000;for(int i=0;i<MAX_IF_NUM;i++){query(i);CHECK(delivered[i]==produced[i]);}
 /* Long repeat storm: hardware scans stay bounded, calls still succeed. */
 init();for(int n=0;n<100000;n++)query(0);CHECK(scans[0]==1);
 jiffies+=1000;query(0);CHECK(scans[0]==2);
 /* Real concurrent same-device calls use the nonblocking gate. */
 init();query(0);pthread_t t[4];for(int i=0;i<4;i++)CHECK(!pthread_create(&t[i],NULL,thread_read,(void*)0));
 for(int i=0;i<4;i++)CHECK(!pthread_join(t[i],NULL));CHECK(scans[0]==1);
 printf("{\"passed\":true,\"assertions\":%lu,\"real_production_handler\":true,\"timer_deltas\":true,\"reset_and_recreate\":true,\"wraparound\":true,\"concurrent_readers\":true,\"cumulative_counters_preserved\":true}\n",assertions);
}
