
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>
typedef uint8_t u8;
typedef int netdev_tx_t;
#define CONFIG_HW_NAT 1
#define IS_ENABLED(x) (x)
#define MTK_SOC_MT6990 1
#define MTK_HAS_CAPS(c,x) ((c)&(x))
#define FOE_INFO_LEN 12
#define TAG_PROTECT 0x55
#define FOE_MAGIC_PPE 0x77
#define UN_HIT 13
#define FOE_TAG_PROTECT(s) ((s)->protect)
#define FOE_MAGIC_TAG(s) ((s)->magic)
#define FOE_AI(s) ((s)->ai)
#define MTK_RESETTING 1
#define SKB_GSO_FRAGLIST 2
#define SKB_GSO_TCPV4 1
#define SKB_GSO_TCPV6 4
#define NETIF_F_ALL_TSO 7
#define NETDEV_TX_BUSY 1
#define NETDEV_TX_OK 0
#define unlikely(x) (x)
#define READ_ONCE(x) (x)
#define IS_ERR(x) ((intptr_t)(x)<0)
#define htons(x) (x)
#define rcu_read_lock() ((void)0)
#define rcu_read_unlock() ((void)0)
#define spin_lock(x) ((void)0)
#define spin_unlock(x) ((void)0)
#define netif_err(...) ((void)0)
#define netif_warn(...) ((void)0)
#define netdev_priv(d) ((d)->priv)
#define atomic_read(p) (*(p))
#define test_bit(b,p) (*(p))
#define skb_list_walk_safe(h,s,n) for((s)=(h),(n)=(s)?(s)->next:0;(s);(s)=(n),(n)=(s)?(s)->next:0)
struct frag {int size;};
struct sk_buff;
struct info {int nr_frags,gso_size,gso_type;struct frag frags[4];struct sk_buff *frag_list;};
struct tcphdr {int check;};
struct sk_buff {int headroom,protect,magic,ai,headlen,pagelen,gso,encapsulation,freed;struct info sh;struct sk_buff *next;struct tcphdr tcp;};
struct net_device_stats {int tx_dropped;};
struct mtk_tx_ring {int free_count,thresh;};
struct soc {int caps,version;};
struct mtk_eth {struct mtk_tx_ring tx_ring;struct soc *soc;int state,page_lock;};
struct mtk_mac {struct mtk_eth *hw;int id;};
struct net_device {struct mtk_mac *priv;struct net_device_stats stats;int features;};
static int routes[64],mapped,seg_error,seg_null,cow_error,hook_result=1;
static struct sk_buff segments[3];
static int skb_headroom(struct sk_buff*s){return s->headroom;}
static int skb_headlen(struct sk_buff*s){return s->headlen;}
static int skb_frag_size(struct frag*f){return f->size;}
static struct info *skb_shinfo(struct sk_buff*s){return &s->sh;}
static int skb_pagelen(struct sk_buff*s){return s->pagelen;}
static int skb_is_gso(struct sk_buff*s){return s->gso;}
static int skb_tcp_all_headers(struct sk_buff*s){return 54;}
static int skb_cow_head(struct sk_buff*s,int n){return cow_error;}
static int skb_vlan_tag_present(struct sk_buff*s){return 0;}
static struct sk_buff *__vlan_hwaccel_push_inside(struct sk_buff*s){return s;}
struct ethhdr {int h_proto;};static struct ethhdr ether;
static struct ethhdr*eth_hdr(struct sk_buff*s){return &ether;}
static int eth_proto_is_802_3(int p){return 1;}
static void netif_tx_stop_all_queues(struct net_device*d){}
static int mtk_cal_txd_req(struct mtk_eth*e,struct sk_buff*s){return 4;}
static int mtk_is_netsys_v1(struct mtk_eth*e){return e->soc->version==1;}
static struct sk_buff*skb_gso_segment(struct sk_buff*s,int f){return seg_error?(void*)(intptr_t)-1:seg_null?0:segments;}
static void consume_skb(struct sk_buff*s){s->freed++;}
static void dev_kfree_skb_any(struct sk_buff*s){s->freed++;}
static int skb_linearize(struct sk_buff*s){return 0;}
static struct tcphdr*tcp_hdr(struct sk_buff*s){return &s->tcp;}
static int hook(struct sk_buff*s,int n){return hook_result;}
static int (*ppe_hook_tx_eth)(struct sk_buff*,int)=hook;
static u8 mtk_mt6990_cpu_wifi_fport(struct mtk_eth *eth, struct sk_buff *skb)
{
#if IS_ENABLED(CONFIG_HW_NAT)
	if (MTK_HAS_CAPS(eth->soc->caps, MTK_SOC_MT6990) &&
	    skb_headroom(skb) >= FOE_INFO_LEN &&
	    FOE_TAG_PROTECT(skb) == TAG_PROTECT &&
	    FOE_MAGIC_TAG(skb) == FOE_MAGIC_PPE && FOE_AI(skb) == UN_HIT) {
		FOE_MAGIC_TAG(skb) = 0;
		return 3; /* SDK MT6990 PPE0 PSE port */
	}
#endif
	return 0;
}
static bool mtk_skb_has_small_frag(struct sk_buff *skb)
{
	int min_size = 16;
	int i;

	if (skb_headlen(skb) < min_size)
		return true;

	for (i = 0; i < skb_shinfo(skb)->nr_frags; i++)
		if (skb_frag_size(&skb_shinfo(skb)->frags[i]) < min_size)
			return true;

	for (skb = skb_shinfo(skb)->frag_list; skb; skb = skb->next)
		if (mtk_skb_has_small_frag(skb))
			return true;

	return false;
}
static bool mtk_skb_valid_gso(struct mtk_eth *eth, struct sk_buff *skb,
			      unsigned int header_len)
{
	if (mtk_is_netsys_v1(eth) && mtk_skb_has_small_frag(skb))
		return false;

	if (!(skb_shinfo(skb)->gso_type & SKB_GSO_FRAGLIST))
		return true;

	if (skb->encapsulation)
		return false;

	return skb_pagelen(skb) - header_len == skb_shinfo(skb)->gso_size &&
	       skb_headlen(skb) > header_len;
}
static int mtk_tx_map(struct sk_buff *skb,struct net_device *dev,int n,struct mtk_tx_ring *r,bool g,unsigned int h, u8 fport){ struct mtk_eth *eth=dev->priv->hw; struct sk_buff *child=skb->sh.frag_list; (void)eth; routes[mapped++]=fport; if(skb->sh.gso_type&SKB_GSO_FRAGLIST)for(;child;child=child->next)routes[mapped++]=fport; return 0;}
static netdev_tx_t mtk_start_xmit(struct sk_buff *skb, struct net_device *dev)
{
	struct mtk_mac *mac = netdev_priv(dev);
	struct mtk_eth *eth = mac->hw;
	struct mtk_tx_ring *ring = &eth->tx_ring;
	struct net_device_stats *stats = &dev->stats;
	struct sk_buff *segs, *next;
	unsigned int header_len = 0;
	bool gso = false;
	u8 fport;
	int tx_num;
#if IS_ENABLED(CONFIG_HW_NAT)
	int (*hnat_hook)(struct sk_buff *, int);
#endif

	if (skb_vlan_tag_present(skb) &&
	    !eth_proto_is_802_3(eth_hdr(skb)->h_proto)) {
		skb = __vlan_hwaccel_push_inside(skb);
		if (!skb)
			goto dropped;
	}

	/* normally we can rely on the stack not calling this more than once,
	 * however we have 2 queues running on the same ring so we need to lock
	 * the ring access
	 */
	spin_lock(&eth->page_lock);

	if (unlikely(test_bit(MTK_RESETTING, &eth->state)))
		goto drop;

#if IS_ENABLED(CONFIG_HW_NAT)
	/* Match the MT6990 SDK contract: Ethernet TX completes binding after
	 * Wi-Fi or modem RX has selected the vendor FOE entry.
	 */
	rcu_read_lock();
	hnat_hook = READ_ONCE(ppe_hook_tx_eth);
	if (MTK_HAS_CAPS(eth->soc->caps, MTK_SOC_MT6990) && hnat_hook &&
	    hnat_hook(skb, mac->id + 1) != 1) {
		rcu_read_unlock();
		goto drop;
	}
	rcu_read_unlock();
#endif

	tx_num = mtk_cal_txd_req(eth, skb);
	if (unlikely(atomic_read(&ring->free_count) <= tx_num)) {
		netif_tx_stop_all_queues(dev);
		netif_err(eth, tx_queued, dev,
			  "Tx Ring full when queue awake!\n");
		spin_unlock(&eth->page_lock);
		return NETDEV_TX_BUSY;
	}

	/* Consume the provider route tag before GSO segmentation/cow can replace
	 * its private headroom. Keep that route for every resulting packet,
	 * including each packet of a GSO frag_list. Do this only after the
	 * NETDEV_TX_BUSY check so a retry retains its original provider tag.
	 */
	fport = mtk_mt6990_cpu_wifi_fport(eth, skb);

	if (skb_is_gso(skb)) {
		header_len = skb_tcp_all_headers(skb);
		if (!mtk_skb_valid_gso(eth, skb, header_len)) {
			segs = skb_gso_segment(skb, dev->features & ~NETIF_F_ALL_TSO);
			if (IS_ERR(segs))
				goto drop;

			if (segs) {
				consume_skb(skb);
				skb = segs;
			}
			goto send;
		}

		if ((skb_shinfo(skb)->gso_type & SKB_GSO_FRAGLIST))
			goto send;

		if (skb_shinfo(skb)->gso_type &
				(SKB_GSO_TCPV4 | SKB_GSO_TCPV6)) {
			/* TSO: fill MSS info in tcp checksum field */
			gso = true;
			if (skb_cow_head(skb, 0)) {
				netif_warn(eth, tx_err, dev,
					   "GSO expand head fail.\n");
				goto drop;
			}

			tcp_hdr(skb)->check = htons(skb_shinfo(skb)->gso_size);
		}
	}

send:
	skb_list_walk_safe(skb, skb, next) {
		if ((mtk_is_netsys_v1(eth) &&
		     mtk_skb_has_small_frag(skb) && skb_linearize(skb)) ||
		    mtk_tx_map(skb, dev, tx_num, ring, gso, header_len, fport) < 0) {
				stats->tx_dropped++;
				dev_kfree_skb_any(skb);
		}
	}

	if (unlikely(atomic_read(&ring->free_count) <= ring->thresh))
		netif_tx_stop_all_queues(dev);

	spin_unlock(&eth->page_lock);

	return NETDEV_TX_OK;

drop:
	spin_unlock(&eth->page_lock);
	dev_kfree_skb_any(skb);
dropped:
	stats->tx_dropped++;
	return NETDEV_TX_OK;
}

static void reset(struct sk_buff*s,struct mtk_eth*e,struct net_device*d){
 memset(s,0,sizeof(*s));memset(segments,0,sizeof(segments));
 for(int i=0;i<3;i++){segments[i].headlen=1514;segments[i].headroom=128;segments[i].next=i<2?&segments[i+1]:0;}
 s->headroom=128;s->protect=TAG_PROTECT;s->magic=FOE_MAGIC_PPE;s->ai=UN_HIT;
 s->headlen=54;s->pagelen=1514;s->sh.gso_size=1460;s->sh.gso_type=SKB_GSO_TCPV4;
 e->soc->caps=MTK_SOC_MT6990;e->soc->version=1;e->tx_ring.free_count=100;e->state=0;
 d->stats.tx_dropped=0;hook_result=1;seg_error=seg_null=cow_error=mapped=0;
}
int main(void){
 struct soc soc={0};struct mtk_eth eth={.soc=&soc};struct mtk_mac mac={.hw=&eth};
 struct net_device dev={.priv=&mac};struct sk_buff skb;int checks=0,regressions=0;
 reset(&skb,&eth,&dev);assert(mtk_start_xmit(&skb,&dev)==NETDEV_TX_OK);assert(mapped==1&&routes[0]==3);checks++;
 reset(&skb,&eth,&dev);skb.magic=0;assert(mtk_start_xmit(&skb,&dev)==NETDEV_TX_OK);assert(mapped==1&&routes[0]==0);checks++;
 reset(&skb,&eth,&dev);soc.caps=0;mtk_start_xmit(&skb,&dev);assert(mapped==1&&routes[0]==0);checks++;
 reset(&skb,&eth,&dev);eth.tx_ring.free_count=1;assert(mtk_start_xmit(&skb,&dev)==NETDEV_TX_BUSY);assert(!mapped&&skb.magic==FOE_MAGIC_PPE&&!skb.freed);checks++;
 reset(&skb,&eth,&dev);hook_result=0;mtk_start_xmit(&skb,&dev);assert(!mapped&&dev.stats.tx_dropped==1);checks++;
 reset(&skb,&eth,&dev);skb.gso=1;skb.sh.nr_frags=1;skb.sh.frags[0].size=1460;mtk_start_xmit(&skb,&dev);assert(mapped==1&&routes[0]==3);checks++;
 reset(&skb,&eth,&dev);skb.gso=1;cow_error=1;mtk_start_xmit(&skb,&dev);assert(!mapped&&dev.stats.tx_dropped==1);checks++;
 for(int size=1;size<=15;size++){
  reset(&skb,&eth,&dev);skb.gso=1;skb.sh.nr_frags=1;skb.sh.frags[0].size=size;
  mtk_start_xmit(&skb,&dev);assert(mapped==3&&skb.freed==1);
  for(int i=0;i<mapped;i++)if(routes[i]!=3)regressions++;
  checks++;
 }
 reset(&skb,&eth,&dev);skb.gso=1;skb.sh.nr_frags=1;skb.sh.frags[0].size=16;
 mtk_start_xmit(&skb,&dev);assert(mapped==1&&routes[0]==3);checks++;
 reset(&skb,&eth,&dev);skb.gso=1;skb.sh.nr_frags=1;skb.sh.frags[0].size=4;seg_error=1;
 mtk_start_xmit(&skb,&dev);assert(!mapped&&dev.stats.tx_dropped==1);checks++;
 reset(&skb,&eth,&dev);skb.gso=1;skb.sh.nr_frags=1;skb.sh.frags[0].size=4;seg_null=1;
 mtk_start_xmit(&skb,&dev);assert(mapped==1&&routes[0]==3);checks++;
 reset(&skb,&eth,&dev);skb.gso=1;skb.headlen=1514;
 skb.sh.gso_type=SKB_GSO_TCPV4|SKB_GSO_FRAGLIST;skb.sh.frag_list=segments;
 mtk_start_xmit(&skb,&dev);assert(mapped==4);
 for(int i=0;i<mapped;i++)if(routes[i]!=3)regressions++;
 checks++;
 printf("{\"control_cases\":%d,\"wrong_segment_routes\":%d}\n",checks,regressions);
 return regressions?3:0;
}
