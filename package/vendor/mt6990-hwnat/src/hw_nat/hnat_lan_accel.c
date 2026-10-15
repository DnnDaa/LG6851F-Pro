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
#include "ra_nat.h"
#include "hnat_define.h"
#include "hnat_common.h"
#include "frame_engine.h"

/* Default enabled; the module parameter allows immediate runtime disable. */
static bool cpu_wifi_accel = true;
module_param(cpu_wifi_accel, bool, 0644);
MODULE_PARM_DESC(cpu_wifi_accel, "Route local unicast TCP/UDP to Wi-Fi through vendor PPE");

#define CPU_WIFI_VID_BASE 0x800
static atomic64_t injected, returned, fallback, xmit_drop;
static struct proc_dir_entry *diagnostics;
static struct proc_dir_entry *flows;
static bool hooks_registered;
static unsigned long first_injection;
extern struct foe_entry *ppe_foe_base, *ppe1_foe_base;

static bool wifi_device(const struct net_device *dev)
{
	return dev && (!strcmp(dev->name, "ra0") || !strcmp(dev->name, "rai0"));
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
	struct net_device *master, *ingress;
	struct sk_buff *copy, *list, *seg, *next, *ready = NULL, **tail = &ready;
	unsigned int index;
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
	if (index == MAX_IF_NUM || DP_GMAC1 >= MAX_IF_NUM)
		return NF_ACCEPT;
	ingress = READ_ONCE(dst_port[DP_GMAC1]);
	if (!ingress || !netif_running(ingress) || !netif_carrier_ok(ingress))
		return NF_ACCEPT;

	/* Keep management reachable while the first real packet makes the loop.
	 * This is passive: no probe traffic or restart is generated.
	 */
	if (READ_ONCE(first_injection) && !atomic64_read(&returned)) {
		if (time_after(jiffies, READ_ONCE(first_injection) + 2 * HZ)) {
			WRITE_ONCE(cpu_wifi_accel, false);
			WRITE_ONCE(first_injection, 0);
			pr_warn_once("HNAT CPU-Wi-Fi: no initial PPE return; using normal Wi-Fi TX\n");
		}
		atomic64_inc(&fallback);
		return NF_ACCEPT;
	}

	/* Build every segment first; any preparation failure keeps the original. */
	copy = skb_copy_expand(skb, FOE_INFO_LEN + ETH_HLEN + VLAN_HLEN, 0, GFP_ATOMIC);
	if (!copy)
		goto failed;
	skb_push(copy, ETH_HLEN);
	skb_reset_mac_header(copy);
	list = copy;
	if (skb_is_gso(copy)) {
		list = skb_gso_segment(copy, 0);
		if (IS_ERR(list)) {
			kfree_skb(copy);
			goto failed;
		}
		if (list)
			consume_skb(copy);
		else
			list = copy;
	}
	for (seg = list; seg; seg = next) {
		next = seg->next;
		seg->next = NULL;
		if (skb_cow_head(seg, FOE_INFO_LEN + VLAN_HLEN) ||
		    (seg->ip_summed == CHECKSUM_PARTIAL && skb_checksum_help(seg))) {
			kfree_skb(seg);
			kfree_skb_list(next);
			goto free_ready;
		}
		seg = vlan_insert_tag(seg, htons(ETH_P_8021Q), CPU_WIFI_VID_BASE | index);
		if (!seg) {
			kfree_skb_list(next);
			goto free_ready;
		}
		memset(seg->head, 0, FOE_INFO_LEN);
		FOE_TAG_PROTECT(seg) = TAG_PROTECT;
		FOE_MAGIC_TAG(seg) = FOE_MAGIC_PPE;
		FOE_AI(seg) = UN_HIT;
		FOE_IF_IDX(seg) = index;
		seg->dev = ingress;
		skb_dst_drop(seg);
		*tail = seg;
		tail = &seg->next;
	}
	consume_skb(skb);
	cmpxchg(&first_injection, 0, jiffies ? jiffies : 1);
	for (seg = ready; seg; seg = next) {
		next = seg->next;
		seg->next = NULL;
		atomic64_inc(&injected);
		ret = dev_queue_xmit(seg);
		if (net_xmit_eval(ret))
			atomic64_inc(&xmit_drop);
	}
	return NF_STOLEN;
free_ready:
	kfree_skb_list(ready);
failed:
	atomic64_inc(&fallback);
	return NF_ACCEPT;
}

/* Called with canonical vendor FOE metadata, before ordinary Ethernet RX. */
bool hnat_cpu_wifi_pingpong(struct sk_buff *skb)
{
	struct vlan_hdr *vlan;
	struct net_device *target, *master;
	unsigned int tag, index;

	/* An internal CPU source port prevents a wire-side VLAN from entering here. */
	if ((FOE_SP(skb) != ADMA_PSE_PORT && FOE_SP(skb) != QDMA_PSE_PORT) ||
	    skb->protocol != htons(ETH_P_8021Q) ||
	    FOE_AI(skb) == HIT_BIND_KEEPALIVE_DUP_OLD_HDR ||
	    !pskb_may_pull(skb, VLAN_HLEN))
		return false;
	vlan = (struct vlan_hdr *)skb->data;
	tag = ntohs(vlan->h_vlan_TCI) & VLAN_VID_MASK;
	if ((tag & ~0x3f) != CPU_WIFI_VID_BASE)
		return false;
	index = tag & 0x3f;
	target = READ_ONCE(dst_port[index]);
	if (!index || !wifi_device(target) || !netif_running(target))
		return false;
	master = netdev_master_upper_dev_get_rcu(target);
	if (!master || !ether_addr_equal(eth_hdr(skb)->h_source, master->dev_addr))
		return false;
	/* The vendor remover cannot replace a caller's skb pointer safely. */
	if (skb_unclone(skb, GFP_ATOMIC) || remove_vlan_tag(skb) != tag) {
		atomic64_inc(&xmit_drop);
		kfree_skb(skb);
		return true;
	}
	skb->dev = target;
	skb->skb_iif = 0;
	FOE_IF_IDX(skb) = index;
	skb_reset_network_header(skb);
	skb_push(skb, ETH_HLEN);
	skb_reset_mac_header(skb);
	skb_dst_drop(skb);
	atomic64_inc(&returned);
	dev_queue_xmit(skb);
	return true;
}

static int lan_accel_show(struct seq_file *s, void *unused)
{
	seq_printf(s, "cpu_wifi_accel %u\ninjected %lld\nreturned %lld\n"
		   "fallback %lld\nxmit_drop %lld\n", READ_ONCE(cpu_wifi_accel),
		   atomic64_read(&injected), atomic64_read(&returned),
		   atomic64_read(&fallback), atomic64_read(&xmit_drop));
	return 0;
}
DEFINE_PROC_SHOW_ATTRIBUTE(lan_accel);

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
	return 0;
}

void hnat_lan_accel_exit(void)
{
	if (hooks_registered)
		nf_unregister_net_hooks(&init_net, hooks, ARRAY_SIZE(hooks));
	proc_remove(diagnostics);
	proc_remove(flows);
}
