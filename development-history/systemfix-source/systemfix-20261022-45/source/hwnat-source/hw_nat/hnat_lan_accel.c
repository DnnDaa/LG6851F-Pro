// SPDX-License-Identifier: GPL-2.0-only
/* MT6990 CPU-to-Wi-Fi PPE ingress, using the vendor FOE/WARP contract. */
#include <linux/module.h>
#include <linux/netfilter_bridge.h>
#include <linux/if_bridge.h>
#include <linux/if_vlan.h>
#include <linux/ip.h>
#include <linux/ipv6.h>
#include <linux/seq_file.h>
#include <linux/proc_fs.h>
#include <linux/jiffies.h>
#include <net/gso.h>
#include <net/ip.h>
#include <net/cfg80211.h>
#include <linux/if_arp.h>
#include <linux/rcupdate.h>
#include <linux/slab.h>
#include "hnat_lan_accel.h"
#include "hnat_wifi_accel_policy.h"
#include "ra_nat.h"
#include "hnat_define.h"
#include "hnat_common.h"
#include "frame_engine.h"

/* Default enabled; the module parameter allows immediate runtime disable. */
static bool cpu_wifi_accel = true;
module_param(cpu_wifi_accel, bool, 0644);
MODULE_PARM_DESC(cpu_wifi_accel, "Route local unicast TCP/UDP to Wi-Fi through vendor PPE");

static atomic64_t keepalive_consumed;
static atomic64_t injected, returned, fallback, xmit_drop, reused, gso_handoff;
static struct proc_dir_entry *diagnostics;
static struct proc_dir_entry *flows;
static bool hooks_registered;
extern struct foe_entry *ppe_foe_base, *ppe1_foe_base;


/* Per-registration identities prevent a returned packet using a recycled AP
 * slot. The device reference lives until all RCU readers have finished. */
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


static bool supported_ip(struct sk_buff *skb)
{
	if (skb->protocol == htons(ETH_P_IP)) {
		const struct iphdr *ip;
		if (!pskb_may_pull(skb, sizeof(*ip)))
			return false;
		ip = ip_hdr(skb);
		return ip->version == 4 && ip->ihl == 5 && !ip_is_fragment(ip) &&
			(ip->protocol == IPPROTO_TCP || ip->protocol == IPPROTO_UDP);
	}
	if (skb->protocol == htons(ETH_P_IPV6)) {
		const struct ipv6hdr *ip;
		if (!pskb_may_pull(skb, sizeof(*ip)))
			return false;
		ip = ipv6_hdr(skb);
		return ip->version == 6 &&
			(ip->nexthdr == IPPROTO_TCP || ip->nexthdr == IPPROTO_UDP);
	}
	return false;
}

static unsigned int cpu_wifi_postrouting(void *unused, struct sk_buff *skb,
					const struct nf_hook_state *state)
{
	struct net_device *master, *ingress, *original_dev;
	unsigned int index, tag;
	struct cpu_wifi_port *port;
	int ret;

	/* Existing cellular, Ethernet, inter-STA and routed traffic are untouched. */
	if (!READ_ONCE(cpu_wifi_accel) || !wifi_device(state->out) || skb->skb_iif ||
	    skb_vlan_tag_present(skb) || !skb_mac_header_was_set(skb) ||
	    !is_valid_ether_addr(eth_hdr(skb)->h_dest))
		return NF_ACCEPT;
	master = netdev_master_upper_dev_get_rcu(state->out);
	if (!master || !netif_is_bridge_master(master) ||
	    !ether_addr_equal(eth_hdr(skb)->h_source, master->dev_addr) ||
	    !supported_ip(skb))
		return NF_ACCEPT;
	for (index = 1; index < MAX_IF_NUM; index++)
		if (READ_ONCE(dst_port[index]) == state->out)
			break;
	if (index >= MAX_IF_NUM || index > 0x3f || DP_GMAC1 >= MAX_IF_NUM)
		return NF_ACCEPT;
	ingress = READ_ONCE(dst_port[DP_GMAC1]);
	if (!ingress || !netif_running(ingress) || !netif_carrier_ok(ingress))
		return NF_ACCEPT;

	rcu_read_lock();
	port = cpu_wifi_find_port(index, state->out);
	if (!port || READ_ONCE(port->disabled)) {
		rcu_read_unlock();
		atomic64_inc(&fallback);
		return NF_ACCEPT;
	}
	tag = cpu_wifi_tag(index, port->generation);
	/* A failed AP probe must not disable acceleration on other SSIDs. */
	if (READ_ONCE(port->first_injection) && !atomic64_read(&port->returned)) {
		if (time_after(jiffies, READ_ONCE(port->first_injection) + 2 * HZ))
			WRITE_ONCE(port->disabled, true);
		atomic64_inc(&port->fallback);
		atomic64_inc(&fallback);
		rcu_read_unlock();
		return NF_ACCEPT;
	}

	/* Match the Redmi CPU-to-PPE ownership model: reuse this skb and its
	 * payload pages. COW only makes the writable head private when needed.
	 * Do not change shared skb objects or enter the frag_list TX path, whose
	 * additional packet heads need separate provider metadata.
	 */
	if (skb_shared(skb) || skb_has_frag_list(skb) ||
	    skb_mac_header(skb) != skb->data - ETH_HLEN ||
	    skb_cow_head(skb, FOE_INFO_LEN + ETH_HLEN + VLAN_HLEN))
		goto failed;

	skb_push(skb, ETH_HLEN);
	skb_reset_mac_header(skb);
	/* Unlike vlan_insert_tag(), this variant never frees the caller's skb.
	 * Its only failure is COW allocation, before inserting any bytes.
	 */
	if (__vlan_insert_tag(skb, htons(ETH_P_8021Q), tag)) {
		skb_pull(skb, ETH_HLEN);
		skb_set_mac_header(skb, -ETH_HLEN);
		goto failed;
	}
	original_dev = skb->dev;
	skb->dev = ingress;
	/* Software GSO would allocate segment heads without the FOE prefix.
	 * Only hand off GSO that this destination can send intact. Unsupported
	 * types keep the ordinary Wi-Fi path with their original headers.
	 */
	if (skb_is_gso(skb) &&
	    (skb_shinfo(skb)->gso_type & SKB_GSO_FRAGLIST ||
	     netif_needs_gso(skb, netif_skb_features(skb)))) {
		skb->dev = original_dev;
		skb_pull(skb, VLAN_HLEN);
		memmove(skb->data, skb->data - VLAN_HLEN, 2 * ETH_ALEN);
		skb_reset_mac_header(skb);
		skb_pull(skb, ETH_HLEN);
		goto failed;
	}
	memset(skb->head, 0, FOE_INFO_LEN);
	FOE_TAG_PROTECT(skb) = TAG_PROTECT;
	FOE_MAGIC_TAG(skb) = FOE_MAGIC_PPE;
	FOE_AI(skb) = UN_HIT;
	FOE_IF_IDX(skb) = index;
	skb_set_queue_mapping(skb, 0);
	skb_dst_drop(skb);
	/* Leave GSO, CHECKSUM_PARTIAL and page frags intact. The destination
	 * netdev validates features and the MT6990 DMA driver handles TSO/CSUM.
	 */
	if (skb_is_gso(skb))
		atomic64_inc(&gso_handoff);
	atomic64_inc(&reused);
	atomic64_inc(&injected);
	atomic64_inc(&port->injected);
	cmpxchg(&port->first_injection, 0, jiffies ? jiffies : 1);
	ret = dev_queue_xmit(skb);
	rcu_read_unlock();
	if (net_xmit_eval(ret))
		atomic64_inc(&xmit_drop);
	return NF_STOLEN;
failed:
	atomic64_inc(&port->fallback);
	rcu_read_unlock();
	atomic64_inc(&fallback);
	return NF_ACCEPT;
}

/* Called with canonical vendor FOE metadata, before ordinary Ethernet RX. */
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

static int lan_accel_show(struct seq_file *s, void *unused)
{
	seq_printf(s, "cpu_wifi_keepalive_consumed %lld\n", atomic64_read(&keepalive_consumed));
	seq_printf(s, "cpu_wifi_accel %u\ninjected %lld\nreturned %lld\n"
		   "fallback %lld\nxmit_drop %lld\nreused %lld\ngso_handoff %lld\n",
		   READ_ONCE(cpu_wifi_accel),
		   atomic64_read(&injected), atomic64_read(&returned),
		   atomic64_read(&fallback), atomic64_read(&xmit_drop),
		   atomic64_read(&reused), atomic64_read(&gso_handoff));
	seq_printf(s, "stale_return %lld\nslot_unavailable %lld\n",
		   atomic64_read(&stale_return), atomic64_read(&slot_unavailable));
	return 0;
}
DEFINE_PROC_SHOW_ATTRIBUTE(lan_accel);

static int lan_ports_show(struct seq_file *s, void *unused)
{
 unsigned int index;
 rcu_read_lock();
 for (index = 1; index < MAX_IF_NUM; index++) {
  struct cpu_wifi_port *port = rcu_dereference(wifi_ports[index]);
  if (port)
   seq_printf(s, "slot %u dev %s ifindex %d epoch %u disabled %u injected %lld returned %lld fallback %lld\n",
              index, port->dev->name, port->dev->ifindex, port->generation,
              READ_ONCE(port->disabled), atomic64_read(&port->injected),
              atomic64_read(&port->returned), atomic64_read(&port->fallback));
 }
 rcu_read_unlock();
 return 0;
}
DEFINE_PROC_SHOW_ATTRIBUTE(lan_ports);

static int lan_flows_show(struct seq_file *s, void *unused)
{
	struct foe_entry *tables[] = { ppe_foe_base, ppe1_foe_base };
	unsigned int ppe, index;
	for (ppe = 0; ppe < ARRAY_SIZE(tables); ppe++) {
		if (!tables[ppe])
			continue;
		for (index = 0; index < FOE_4TB_SIZ; index++) {
			struct foe_entry *entry = &tables[ppe][index];
			__be32 src, dst;
			unsigned int port;
			if (!ppe_entry_check_state(entry, BIND) || !IS_IPV4_GRP(entry))
				continue;
			port = entry->ipv4_hnapt.iblk2.dp;
			if (port != WDMA0_PSE_PORT && port != WDMA1_PSE_PORT)
				continue;
			src = htonl(entry->ipv4_hnapt.sip);
			dst = htonl(entry->ipv4_hnapt.dip);
			seq_printf(s, "ppe %u index %u BIND %pI4:%u -> %pI4:%u dp %u wcid %u bss %u\n",
				   ppe, index, &src, entry->ipv4_hnapt.sport,
				   &dst, entry->ipv4_hnapt.dport, port,
				   (entry->ipv4_hnapt.winfo >> 6) & 0x3ff,
				   entry->ipv4_hnapt.winfo & 0x3f);
		}
	}
	return 0;
}
DEFINE_PROC_SHOW_ATTRIBUTE(lan_flows);

static const struct nf_hook_ops hooks[] = {
	{
		.hook = cpu_wifi_postrouting,
		.pf = NFPROTO_BRIDGE,
		.hooknum = NF_BR_POST_ROUTING,
		.priority = NF_BR_PRI_LAST - 2,
	},
};

int hnat_lan_accel_init(void)
{
	int ret = nf_register_net_hooks(&init_net, hooks, ARRAY_SIZE(hooks));
	if (ret)
		return ret;
	hooks_registered = true;
	diagnostics = proc_create("mt6990_lan_accel", 0400, NULL, &lan_accel_proc_ops);
	flows = proc_create("mt6990_lan_flows", 0400, NULL, &lan_flows_proc_ops);
	ports = proc_create("mt6990_lan_ports", 0400, NULL, &lan_ports_proc_ops);
	return 0;
}

void hnat_lan_accel_exit(void)
{
	unsigned long flags;
	spin_lock_irqsave(&wifi_ports_lock, flags);
	WRITE_ONCE(cpu_wifi_stopping, true);
	spin_unlock_irqrestore(&wifi_ports_lock, flags);
	if (hooks_registered)
		nf_unregister_net_hooks(&init_net, hooks, ARRAY_SIZE(hooks));
	proc_remove(diagnostics);
	proc_remove(flows);
	proc_remove(ports);
	hnat_cpu_wifi_port_down(NULL);
	rcu_barrier();
}
