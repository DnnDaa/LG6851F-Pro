
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <limits.h>
#include <arpa/inet.h>
typedef uint8_t u8;
#define HZ 100
#define MAX_IF_NUM 64
#define GFP_ATOMIC 0
#define ARPHRD_ETHER 1
#define NL80211_IFTYPE_AP 3
#define NL80211_IFTYPE_STATION 2
#define __rcu
#define READ_ONCE(x) (x)
#define WRITE_ONCE(x,v) ((x)=(v))
#define DEFINE_SPINLOCK(x) int x
#define spin_lock_irqsave(l,f) ((void)(l),(f)=0)
#define spin_unlock_irqrestore(l,f) ((void)(l),(void)(f))
#define lockdep_is_held(x) true
#define rcu_dereference_protected(x,held) (x)
#define rcu_dereference(x) (x)
#define RCU_INIT_POINTER(x,v) ((x)=(v))
#define rcu_assign_pointer(x,v) ((x)=(v))
#define time_after_eq(a,b) ((long)((a)-(b))>=0)
#define container_of(ptr,type,member) ((type *)((char *)(ptr)-offsetof(type,member)))
#define kzalloc(n,g) calloc(1,n)
#define kfree free
typedef struct { long long value; } atomic64_t;
static void atomic64_inc(atomic64_t *a){a->value++;}
static long long atomic64_read(const atomic64_t *a){return a->value;}
struct rcu_head{struct rcu_head *next;void (*fn)(struct rcu_head *);};
static struct rcu_head *deferred;
static unsigned int readers;
static void call_rcu(struct rcu_head *h,void (*f)(struct rcu_head *)){h->fn=f;h->next=deferred;deferred=h;}
static void rcu_read_lock(void){readers++;}
static void rcu_read_unlock(void){assert(readers);readers--;}
static void rcu_barrier(void){assert(!readers);while(deferred){struct rcu_head *h=deferred;deferred=h->next;h->fn(h);}}
struct net{int value;};static struct net init_net,other_net;
struct wireless_dev{void *wiphy;int iftype;};
struct net_device{char name[32];struct wireless_dev *ieee80211_ptr;int type,up,refs,ifindex;struct net *net;struct net_device *master;u8 dev_addr[6];};
static struct net_device *dst_port[MAX_IF_NUM];
static u8 dst_port_type[MAX_IF_NUM];
#define DP_CCMNI0 40
#define SW_PATH 1
#define HW_PATH 0
#define pr_notice(...) ((void)0)
static int kstrtoint(const char *s,int base,int *v){char *e;long n=strtol(s,&e,base);if(*e||n<INT_MIN||n>INT_MAX)return -1;*v=n;return 0;}
static int reset_count,flush_count;
static void ppe_reset_dev_mib(struct net_device *d){(void)d;reset_count++;}
static void foe_del_entry_by_dev(struct net_device *d){(void)d;flush_count++;}
static struct net *dev_net(const struct net_device *d){return d->net;}
static bool net_eq(const struct net *a,const struct net *b){return a==b;}
static void dev_hold(struct net_device *d){d->refs++;}
static void dev_put(struct net_device *d){assert(d->refs>1);d->refs--;}
static bool netif_running(const struct net_device *d){return d->up;}
static unsigned long jiffies=1000;
struct proc_dir_entry{};
#define ETH_HLEN 14
#define VLAN_HLEN 4
#define VLAN_VID_MASK 0xfff
#define ETH_P_8021Q 0x8100
#define ADMA_PSE_PORT 0
#define QDMA_PSE_PORT 5
#define HIT_BIND_KEEPALIVE_DUP_OLD_HDR 19
struct vlan_hdr{uint16_t h_vlan_TCI,h_vlan_encapsulated_proto;};
struct ethhdr{u8 h_dest[6],h_source[6];uint16_t h_proto;};
struct sk_buff{struct net_device *dev;unsigned int skb_iif,source,reason,if_idx;uint16_t protocol,vlan_tag;bool hw_vlan,mac_set;u8 buffer[80];u8 *data;struct ethhdr eth;};
#define FOE_SP(s) ((s)->source)
#define FOE_AI(s) ((s)->reason)
#define FOE_IF_IDX(s) ((s)->if_idx)
static atomic64_t keepalive_consumed,returned,xmit_drop;
static unsigned int transmitted,consumed;
static struct net_device *transmit_dev;
static bool skb_mac_header_was_set(struct sk_buff *s){return s->mac_set;}
static bool skb_vlan_tag_present(struct sk_buff *s){return s->hw_vlan;}
static unsigned int skb_vlan_tag_get(struct sk_buff *s){return s->vlan_tag;}
static bool pskb_may_pull(struct sk_buff *s,unsigned int n){return n<=sizeof(s->buffer);}
static struct net_device *netdev_master_upper_dev_get_rcu(struct net_device *d){return d->master;}
static bool netif_is_bridge_master(struct net_device *d){return d&&d->ifindex==1;}
static bool ether_addr_equal(const u8 *a,const u8 *b){return memcmp(a,b,6)==0;}
static struct ethhdr *eth_hdr(struct sk_buff *s){return &s->eth;}
static void consume_skb(struct sk_buff *s){(void)s;consumed++;}
static unsigned int remove_vlan_tag(struct sk_buff *s){unsigned int t=s->hw_vlan?s->vlan_tag:ntohs(((struct vlan_hdr *)s->data)->h_vlan_TCI);s->hw_vlan=false;return t&VLAN_VID_MASK;}
static void skb_reset_network_header(struct sk_buff *s){(void)s;}
static void skb_push(struct sk_buff *s,unsigned int n){s->data-=n;}
static void skb_reset_mac_header(struct sk_buff *s){(void)s;}
static void skb_dst_drop(struct sk_buff *s){(void)s;}
static int dev_queue_xmit(struct sk_buff *s){transmitted++;transmit_dev=s->dev;return 0;}
/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef LG6851F_WIFI_ACCEL_POLICY_H
#define LG6851F_WIFI_ACCEL_POLICY_H
/* The provider ABI is six bits, not the AP's SSID number or ifindex. */
#define CPU_WIFI_INDEX_BITS 6
#define CPU_WIFI_INDEX_MASK ((1U << CPU_WIFI_INDEX_BITS) - 1)
#define CPU_WIFI_GENERATION_MASK 31U
#define CPU_WIFI_VID_BASE 0x800
#define CPU_WIFI_GENERATION_GUARD (30 * HZ)

static inline bool wifi_device(const struct net_device *dev)
{
 const struct wireless_dev *wdev = dev ? dev->ieee80211_ptr : NULL;
 return wdev && wdev->wiphy && wdev->iftype == NL80211_IFTYPE_AP &&
        dev->type == ARPHRD_ETHER && net_eq(dev_net(dev), &init_net);
}

static inline unsigned int cpu_wifi_tag(unsigned int index, unsigned int generation)
{
 return CPU_WIFI_VID_BASE | (generation << CPU_WIFI_INDEX_BITS) | index;
}
#endif
struct cpu_wifi_port {
 struct net_device *dev;
 struct rcu_head rcu;
 u8 generation;
 atomic64_t injected, returned, fallback;
 unsigned long first_injection;
 bool disabled;
};
static struct cpu_wifi_port __rcu *wifi_ports[MAX_IF_NUM];
static DEFINE_SPINLOCK(wifi_ports_lock);
static u8 wifi_generation[MAX_IF_NUM];
static unsigned long wifi_retired[MAX_IF_NUM][CPU_WIFI_GENERATION_MASK + 1];
static bool wifi_retired_valid[MAX_IF_NUM][CPU_WIFI_GENERATION_MASK + 1];
static atomic64_t stale_return, slot_unavailable;
static struct proc_dir_entry *ports;
static bool cpu_wifi_stopping;

static void cpu_wifi_port_free(struct rcu_head *head)
{
 struct cpu_wifi_port *port = container_of(head, struct cpu_wifi_port, rcu);
 dev_put(port->dev);
 kfree(port);
}

void hnat_cpu_wifi_port_register(unsigned int index, struct net_device *dev)
{
 struct cpu_wifi_port *port, *old;
 unsigned long flags;
 unsigned int n, generation;
 if (READ_ONCE(cpu_wifi_stopping) || !index || index >= MAX_IF_NUM || index > CPU_WIFI_INDEX_MASK || !wifi_device(dev))
  return;
 port = kzalloc(sizeof(*port), GFP_ATOMIC);
 if (!port) {
  atomic64_inc(&slot_unavailable);
  return;
 }
 dev_hold(dev);
 port->dev = dev;
 spin_lock_irqsave(&wifi_ports_lock, flags);
 if (cpu_wifi_stopping) {
  spin_unlock_irqrestore(&wifi_ports_lock, flags);
  dev_put(dev);
  kfree(port);
  return;
 }
 old = rcu_dereference_protected(wifi_ports[index], lockdep_is_held(&wifi_ports_lock));
 if (old && old->dev == dev) {
  spin_unlock_irqrestore(&wifi_ports_lock, flags);
  dev_put(dev);
  kfree(port);
  return;
 }
 /* Do not wrap an incarnation into the lifetime of a delayed DMA return.
  * Exhaustion keeps normal networking; it never aliases a different BSS. */
 for (n = 1; n <= CPU_WIFI_GENERATION_MASK + 1; n++) {
  generation = (wifi_generation[index] + n) & CPU_WIFI_GENERATION_MASK;
  if ((!old || generation != old->generation) &&
      (!wifi_retired_valid[index][generation] ||
       time_after_eq(jiffies, wifi_retired[index][generation] + CPU_WIFI_GENERATION_GUARD)))
   break;
 }
 if (n > CPU_WIFI_GENERATION_MASK + 1) {
  spin_unlock_irqrestore(&wifi_ports_lock, flags);
  dev_put(dev);
  kfree(port);
  atomic64_inc(&slot_unavailable);
  return;
 }
 port->generation = generation;
 wifi_generation[index] = generation;
 if (old) {
  wifi_retired[index][old->generation] = jiffies;
  wifi_retired_valid[index][old->generation] = true;
 }
 rcu_assign_pointer(wifi_ports[index], port);
 if (old)
  call_rcu(&old->rcu, cpu_wifi_port_free);
 spin_unlock_irqrestore(&wifi_ports_lock, flags);
}

void hnat_cpu_wifi_port_down(struct net_device *dev)
{
 unsigned long flags;
 unsigned int index;
 for (index = 1; index < MAX_IF_NUM; index++) {
  struct cpu_wifi_port *old;
  spin_lock_irqsave(&wifi_ports_lock, flags);
  old = rcu_dereference_protected(wifi_ports[index], lockdep_is_held(&wifi_ports_lock));
  if (old && (!dev || old->dev == dev)) {
   wifi_retired[index][old->generation] = jiffies;
   wifi_retired_valid[index][old->generation] = true;
   RCU_INIT_POINTER(wifi_ports[index], NULL);
  } else {
   old = NULL;
  }
  if (old)
   call_rcu(&old->rcu, cpu_wifi_port_free);
  spin_unlock_irqrestore(&wifi_ports_lock, flags);
 }
}

static struct cpu_wifi_port *cpu_wifi_find_port(unsigned int index, struct net_device *dev)
{
 struct cpu_wifi_port *port;
 if (!index || index >= MAX_IF_NUM || index > CPU_WIFI_INDEX_MASK)
  return NULL;
 port = rcu_dereference(wifi_ports[index]);
 if (!port || port->dev != dev || READ_ONCE(dst_port[index]) != dev ||
     !wifi_device(dev) || !netif_running(dev))
  return NULL;
 return port;
}


bool hnat_cpu_wifi_pingpong(struct sk_buff *skb)
{
 struct vlan_hdr *vlan;
 struct net_device *target, *master;
 struct cpu_wifi_port *port;
 unsigned int tag, index, generation;

 if ((FOE_SP(skb) != ADMA_PSE_PORT && FOE_SP(skb) != QDMA_PSE_PORT) ||
     !skb_mac_header_was_set(skb))
  return false;
 if (skb_vlan_tag_present(skb)) {
  tag = skb_vlan_tag_get(skb) & VLAN_VID_MASK;
 } else {
  if (skb->protocol != htons(ETH_P_8021Q) || !pskb_may_pull(skb, VLAN_HLEN))
   return false;
  vlan = (struct vlan_hdr *)skb->data;
  tag = ntohs(vlan->h_vlan_TCI) & VLAN_VID_MASK;
 }
 if (!(tag & CPU_WIFI_VID_BASE))
  return false;
 index = tag & CPU_WIFI_INDEX_MASK;
 generation = (tag >> CPU_WIFI_INDEX_BITS) & CPU_WIFI_GENERATION_MASK;
 rcu_read_lock();
 port = (index && index < MAX_IF_NUM) ? rcu_dereference(wifi_ports[index]) : NULL;
 target = port ? port->dev : NULL;
 if (!port || port->generation != generation ||
     cpu_wifi_find_port(index, target) != port)
  goto stale;
 master = netdev_master_upper_dev_get_rcu(target);
 if (!master || !netif_is_bridge_master(master) ||
     !ether_addr_equal(eth_hdr(skb)->h_source, master->dev_addr))
  goto stale;
 if (FOE_AI(skb) == HIT_BIND_KEEPALIVE_DUP_OLD_HDR) {
  atomic64_inc(&keepalive_consumed);
  rcu_read_unlock();
  consume_skb(skb);
  return true;
 }
 if (remove_vlan_tag(skb) != tag)
  goto stale;
 skb->dev = target;
 skb->skb_iif = 0;
 FOE_IF_IDX(skb) = index;
 skb_reset_network_header(skb);
 skb_push(skb, ETH_HLEN);
 skb_reset_mac_header(skb);
 skb_dst_drop(skb);
 atomic64_inc(&port->returned);
 atomic64_inc(&returned);
 dev_queue_xmit(skb);
 rcu_read_unlock();
 return true;
stale:
 /* An internal return for a removed/replaced AP is never ordinary ingress. */
 atomic64_inc(&stale_return);
 atomic64_inc(&xmit_drop);
 rcu_read_unlock();
 consume_skb(skb);
 return true;
}

void ppe_dev_reg_handler(struct net_device *dev)
{
	int i, err;


	if (dev == NULL)
		return;

	if (strncmp(dev->name, "ccmni", 5) == 0) {

		err = kstrtoint(&dev->name[5], 10, &i);
		if (err != 0) {
			pr_notice("%s, err: %d\n", __func__, err);
			return;
		}

		if (i < 0 || i >= MAX_IF_NUM - DP_CCMNI0)
			return;
		i += DP_CCMNI0;

		if (i < 0 || i >= MAX_IF_NUM) {
			pr_notice("%s, err, %d is out of bound\n", __func__, i);
			return;
		}

		if (dst_port[i] == dev) {
			hnat_cpu_wifi_port_register(i, dev);
			pr_notice("%s, (%d)%s - redundant\n", __func__, i, dev->name);
			return;

		} else if (dst_port[i] == NULL) {
			dst_port[i] = dev;
			dst_port_type[i] = HW_PATH;

			pr_notice("%s, (%d)%s - succeed\n", __func__, i, dev->name);

		} else {

			pr_notice("%s, ERR (%d)%s - occupied, check why ?\n", __func__, i, dev->name);
			return;
		}

	} else {

		for (i = 1; i < MAX_IF_NUM; i++) {

			if (dst_port[i] == dev) {
			hnat_cpu_wifi_port_register(i, dev);
				pr_notice("%s, (%d)%s - redundant\n", __func__, i, dev->name);
				return;
			}
		}

		for (i = 1; i < MAX_IF_NUM && i < DP_CCMNI0; i++) {

			if (dst_port[i] == NULL) {
				dst_port[i] = dev;
				dst_port_type[i] = SW_PATH;
			hnat_cpu_wifi_port_register(i, dev);

				pr_notice("%s, (%d)%s - succeed\n", __func__, i, dev->name);
				break;
			}
		}
	}
}

void ppe_dev_unreg_handler(struct net_device *dev)
{
	int i, del = 0;

	if (dev == NULL)
		return;

	hnat_cpu_wifi_port_down(dev);
	for (i = 1; i < MAX_IF_NUM; i++) {
		if (dst_port[i] == dev) {
			ppe_reset_dev_mib(dev);
			foe_del_entry_by_dev(dev);
			dst_port[i] = NULL;
			del = 1;
			break;
		}
	}

	if (del == 1)
		pr_notice("%s, (%d)%s - removed\n", __func__, i, dev->name);

}

static unsigned int checks;
#define check(x) do{checks++;assert(x);}while(0)
static struct sk_buff packet(unsigned int tag,struct net_device *bridge){
 struct sk_buff p={.source=ADMA_PSE_PORT,.mac_set=true,.hw_vlan=true,.vlan_tag=tag,.protocol=htons(ETH_P_8021Q)};
 memcpy(p.eth.h_source,bridge->dev_addr,6);return p;
}
int main(void){
 struct wireless_dev ap={.wiphy=(void *)1,.iftype=NL80211_IFTYPE_AP};
 struct wireless_dev station={.wiphy=(void *)1,.iftype=NL80211_IFTYPE_STATION};
 struct net_device bridge={.name="br-lan",.ifindex=1,.dev_addr={2,0,0,0,0,1}};
 struct net_device a={.name="ra1",.ieee80211_ptr=&ap,.type=ARPHRD_ETHER,.up=1,.refs=1,.ifindex=99,.net=&init_net,.master=&bridge};
 struct net_device b=a;b.ifindex=100;strcpy(b.name,"renamed-ap");
 for(int i=0;i<100;i++){snprintf(a.name,sizeof(a.name),"arbitrary-ap-%d",i);check(wifi_device(&a));}
 a.ieee80211_ptr=&station;check(!wifi_device(&a));a.ieee80211_ptr=&ap;
 a.net=&other_net;check(!wifi_device(&a));a.net=&init_net;
 check(!wifi_device(NULL));
 dst_port[7]=&a;hnat_cpu_wifi_port_register(7,&a);
 check(wifi_ports[7]&&a.refs==2);
 struct cpu_wifi_port *first=wifi_ports[7];unsigned int oldgen=first->generation;unsigned int oldtag=cpu_wifi_tag(7,oldgen);
 hnat_cpu_wifi_port_register(7,&a);check(first==wifi_ports[7]&&a.refs==2);
 struct sk_buff p=packet(oldtag,&bridge);p.data=p.buffer+32;
 check(hnat_cpu_wifi_pingpong(&p)&&transmitted==1&&transmit_dev==&a&&p.if_idx==7);
 check(atomic64_read(&first->returned)==1&&readers==0);
 rcu_read_lock();hnat_cpu_wifi_port_down(&a);check(wifi_ports[7]==NULL&&a.refs==2);rcu_read_unlock();rcu_barrier();check(a.refs==1);
 dst_port[7]=&b;hnat_cpu_wifi_port_register(7,&b);
 check(wifi_ports[7]&&wifi_ports[7]->generation!=oldgen); /* first is intentionally not dereferenced below */
 p=packet(oldtag,&bridge);p.data=p.buffer+32;unsigned int before=transmitted;
 check(hnat_cpu_wifi_pingpong(&p)&&transmitted==before&&consumed==1);
 unsigned int tag=cpu_wifi_tag(7,wifi_ports[7]->generation);
 p=packet(tag,&bridge);p.data=p.buffer+32;check(hnat_cpu_wifi_pingpong(&p)&&transmit_dev==&b);
 p=packet(tag,&bridge);p.data=p.buffer+32;p.reason=HIT_BIND_KEEPALIVE_DUP_OLD_HDR;before=transmitted;
 check(hnat_cpu_wifi_pingpong(&p)&&transmitted==before&&atomic64_read(&keepalive_consumed)==1);
 p=packet(tag,&bridge);p.data=p.buffer+32;p.source=7;before=consumed;
 check(!hnat_cpu_wifi_pingpong(&p)&&consumed==before);
 p=packet(100,&bridge);p.data=p.buffer+32;check(!hnat_cpu_wifi_pingpong(&p));
 p=packet(tag,&bridge);p.data=p.buffer+32;p.eth.h_source[0]=4;before=transmitted;
 check(hnat_cpu_wifi_pingpong(&p)&&transmitted==before);
 p=packet(tag,&bridge);p.data=p.buffer+32;p.hw_vlan=false;((struct vlan_hdr *)p.data)->h_vlan_TCI=htons(tag);
 check(hnat_cpu_wifi_pingpong(&p)&&transmit_dev==&b);
 /* A replaced device must not be accepted simply because an index exists. */
 dst_port[7]=&a;p=packet(tag,&bridge);p.data=p.buffer+32;before=transmitted;
 check(hnat_cpu_wifi_pingpong(&p)&&transmitted==before);dst_port[7]=&b;
 for(unsigned int i=0;i<40;i++){hnat_cpu_wifi_port_down(&b);hnat_cpu_wifi_port_register(7,&b);}
 check(atomic64_read(&slot_unavailable)>0&&wifi_ports[7]==NULL);
 jiffies+=CPU_WIFI_GENERATION_GUARD+1;hnat_cpu_wifi_port_register(7,&b);check(wifi_ports[7]!=NULL);
 hnat_cpu_wifi_port_register(64,&a);hnat_cpu_wifi_port_register(0,&a);check(a.refs==1);
 /* The failure/probe state belongs to one AP, not the global switch. */
 dst_port[9]=&a;hnat_cpu_wifi_port_register(9,&a);wifi_ports[7]->disabled=true;check(!wifi_ports[9]->disabled);
 cpu_wifi_stopping=true;hnat_cpu_wifi_port_down(NULL);hnat_cpu_wifi_port_register(9,&a);check(wifi_ports[9]==NULL);
 rcu_barrier();check(a.refs==1&&b.refs==1&&!readers);
 /* Exercise the actual provider allocator, including its reserved modem slots. */
 cpu_wifi_stopping=false;memset(dst_port,0,sizeof(dst_port));
 jiffies+=CPU_WIFI_GENERATION_GUARD+1;
 struct net_device many[100];
 for(unsigned int i=0;i<100;i++){
  many[i]=a;snprintf(many[i].name,sizeof(many[i].name),"new-ap-%u",i);many[i].refs=1;many[i].ifindex=200+i;
  ppe_dev_reg_handler(&many[i]);
 }
 for(unsigned int i=1;i<DP_CCMNI0;i++)check(dst_port[i]==&many[i-1]&&wifi_ports[i]&&wifi_ports[i]->dev==&many[i-1]);
 for(unsigned int i=DP_CCMNI0;i<MAX_IF_NUM;i++)check(dst_port[i]==NULL);
 struct net_device modem={.name="ccmni0",.refs=1};ppe_dev_reg_handler(&modem);check(dst_port[40]==&modem);
 struct net_device invalid={.name="ccmni2147483647",.refs=1};ppe_dev_reg_handler(&invalid);check(dst_port[63]==NULL);
 ppe_dev_unreg_handler(&many[0]);check(dst_port[1]==NULL&&wifi_ports[1]==NULL&&reset_count==1&&flush_count==1);
 rcu_barrier();check(many[0].refs==1);
 ppe_dev_reg_handler(&many[99]);check(dst_port[1]==&many[99]&&wifi_ports[1]->dev==&many[99]);
 cpu_wifi_stopping=true;hnat_cpu_wifi_port_down(NULL);rcu_barrier();
 for(unsigned int i=0;i<100;i++)check(many[i].refs==1);
 printf("{\"passed\":true,\"actual_policy_registration_return_checks\":%u,\"mocked_kernel_hardware\":true}\n",checks);
 return 0;
}
