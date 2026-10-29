
#include <assert.h>
#include <arpa/inet.h>
#include <errno.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
typedef uint32_t __be32;typedef uint16_t __be16;typedef uint32_t u32;typedef uint16_t u_int16_t;
#define NF_ACCEPT 1
#define NF_DROP 0
#define NF_INET_POST_ROUTING 4
#define NF_NAT_RANGE_MAP_IPS 1
#define NF_NAT_RANGE_PROTO_SPECIFIED 2
#define NF_NAT_RANGE_PROTO_RANDOM_FULLY (1<<4)
#define NF_NAT_MANIP_SRC 0
#define RT_SCOPE_UNIVERSE 0
#define GFP_ATOMIC 0
#define READ_ONCE(x) (x)
#define WRITE_ONCE(x,v) ((x)=(v))
#define WARN_ON(x) (x)
#define WARN_ON_ONCE(x) (x)
#define pr_info(...) ((void)0)
#define spin_lock_bh(x) ((void)(x))
#define spin_unlock_bh(x) ((void)(x))
#define rcu_read_lock() ((void)0)
#define rcu_read_unlock() ((void)0)
#define rcu_access_pointer(x) (x)
enum ip_conntrack_info {IP_CT_NEW,IP_CT_RELATED,IP_CT_RELATED_REPLY};
enum {IP_CT_DIR_ORIGINAL=0,IP_CT_DIR_REPLY=1};
union nf_inet_addr {__be32 ip;uint32_t all[4];};
union nf_conntrack_man_proto {__be16 all;struct{__be16 port;} udp;};
struct nf_conntrack_tuple {
 struct {union nf_inet_addr u3;union nf_conntrack_man_proto u;int l3num;} src;
 struct {union nf_inet_addr u3;union nf_conntrack_man_proto u;uint8_t protonum;} dst;
};
struct nf_conntrack_helper{int dummy;};
struct nf_conn_help{struct nf_conntrack_helper *helper;};
struct nf_conn_nat{int masq_index;};
struct nf_conn {
 struct {struct nf_conntrack_tuple tuple;} tuplehash[2];
 struct nf_conn_nat nat;struct nf_conn_help help;bool hashelp;int net,zone;
};
struct nf_conntrack_expect {
 struct nf_conntrack_expect *next;struct nf_conn *master;
 struct nf_conntrack_tuple tuple;union nf_inet_addr saved_addr;union nf_conntrack_man_proto saved_proto;
 void (*expectfn)(struct nf_conn *,struct nf_conntrack_expect *);
};
struct hlist_head{struct nf_conntrack_expect *first;};
static struct hlist_head buckets[4];static struct hlist_head *nf_ct_expect_hash=buckets;
static unsigned int nf_ct_expect_hsize=4;
#define hlist_for_each_entry_rcu(p,h,unused) for((p)=(h)->first;(p);(p)=(p)->next)
struct nf_nat_range2{unsigned int flags;union nf_inet_addr min_addr,max_addr;union nf_conntrack_man_proto min_proto,max_proto;};
struct net_device{int ifindex;char *name;};
struct sk_buff{struct nf_conn *ct;};struct rtable{int dummy;};struct iphdr{__be32 daddr;};
static struct rtable route;static struct iphdr iphead;static __be32 selected_ip;
static uint8_t occupied[65536];static unsigned probes;static struct nf_conntrack_expect dummy_expect;
static struct nf_nat_range2 captured;static unsigned setups;static unsigned int lg6851f_nat_mode;
static int nf_conntrack_expect_lock;static struct nf_conntrack_helper nf_conntrack_helper_bcm_nat;
static void bcm_nat_expect(struct nf_conn *ct,struct nf_conntrack_expect *exp){(void)ct;(void)exp;}
static int nf_ct_net(struct nf_conn *ct){return ct->net;}
static int nf_ct_zone(struct nf_conn *ct){return ct->zone;}
static bool nf_ct_zone_equal_any(struct nf_conn *ct,int zone){return ct->zone==zone;}
static bool net_eq(int a,int b){return a==b;}
static bool nf_inet_addr_cmp(const union nf_inet_addr *a,const union nf_inet_addr *b){return !memcmp(a,b,sizeof(*a));}
static struct nf_conntrack_expect *__nf_ct_expect_find(int net,int zone,const struct nf_conntrack_tuple *t){(void)net;(void)zone;probes++;return occupied[ntohs(t->dst.u.udp.port)]?&dummy_expect:NULL;}
static struct nf_conn *nf_ct_get(struct sk_buff *skb,enum ip_conntrack_info *info){*info=IP_CT_NEW;return skb->ct;}
static struct rtable *skb_rtable(struct sk_buff *skb){(void)skb;return &route;}
static struct iphdr *ip_hdr(struct sk_buff *skb){(void)skb;return &iphead;}
static __be32 rt_nexthop(const struct rtable *rt,__be32 dst){(void)rt;return dst;}
static __be32 inet_select_addr(const struct net_device *dev,__be32 nh,int scope){(void)dev;(void)nh;(void)scope;return selected_ip;}
static struct nf_conn_nat *nf_ct_nat_ext_add(struct nf_conn *ct){return &ct->nat;}
static struct nf_conn_help *nfct_help(struct nf_conn *ct){return ct->hashelp?&ct->help:NULL;}
static struct nf_conn_help *nf_ct_helper_ext_add(struct nf_conn *ct,int flags){(void)flags;ct->hashelp=true;return &ct->help;}
static int nf_ct_protonum(struct nf_conn *ct){return ct->tuplehash[0].tuple.dst.protonum;}
static unsigned int nf_nat_setup_info(struct nf_conn *ct,const struct nf_nat_range2 *r,int manip){(void)ct;(void)manip;captured=*r;setups++;return NF_ACCEPT;}
struct kernel_param{void *arg;};
static int kstrtouint(const char *s,int base,unsigned int *out){char *end;unsigned long n;if(!s[0]||s[0]=='-')return -EINVAL;errno=0;n=strtoul(s,&end,base);if(errno||*end||n>UINT32_MAX)return -EINVAL;*out=n;return 0;}

static int lg6851f_set_nat_mode(const char *value, const struct kernel_param *kp)
{
 unsigned int mode;
 int err = kstrtouint(value, 0, &mode);
 if (err || mode > 2)
  return err ?: -EINVAL;
 WRITE_ONCE(*(unsigned int *)kp->arg, mode);
 return 0;
}

static inline int find_exp(__be32 ip, __be16 port, struct nf_conn *ct)
{
       struct nf_conntrack_tuple tuple;
       struct nf_conntrack_expect *i = NULL;


       memset(&tuple, 0, sizeof(tuple));
       tuple.src.l3num = AF_INET;
       tuple.dst.protonum = IPPROTO_UDP;
       tuple.dst.u3.ip = ip;
       tuple.dst.u.udp.port = port;

       rcu_read_lock();
       i = __nf_ct_expect_find(nf_ct_net(ct), nf_ct_zone(ct), &tuple);
       rcu_read_unlock();

       return i != NULL;
}

static inline struct nf_conntrack_expect *find_fullcone_exp(struct nf_conn *ct, __be32 external_ip)
{
       struct nf_conntrack_tuple * tp =
               &ct->tuplehash[IP_CT_DIR_ORIGINAL].tuple;
       struct nf_conntrack_expect * exp = NULL;
       struct nf_conntrack_expect * i;
       unsigned int h;

       rcu_read_lock();
       for (h = 0; h < nf_ct_expect_hsize; h++) {
               hlist_for_each_entry_rcu(i, &nf_ct_expect_hash[h], hnode) {
                       if (i->expectfn == bcm_nat_expect &&
                           net_eq(nf_ct_net(i->master), nf_ct_net(ct)) &&
                           nf_ct_zone_equal_any(i->master, nf_ct_zone(ct)) &&
                           i->tuple.dst.u3.ip == external_ip &&
                           nf_inet_addr_cmp(&i->saved_addr, &tp->src.u3) &&
                           i->saved_proto.all == tp->src.u.all &&
                           i->tuple.dst.protonum == tp->dst.protonum &&
                           i->tuple.src.u3.ip == 0 &&
                           i->tuple.src.u.udp.port == 0) {
                               exp = i;
                               break;
                       }
               }
       }
       rcu_read_unlock();

       return exp;
}

unsigned int
nf_nat_masquerade_ipv4(struct sk_buff *skb, unsigned int hooknum,
		       const struct nf_nat_range2 *range,
		       const struct net_device *out)
{
	struct nf_conn *ct;
	struct nf_conn_nat *nat;
	enum ip_conntrack_info ctinfo;
	struct nf_nat_range2 newrange;
	const struct rtable *rt;
	__be32 newsrc, nh;

	WARN_ON(hooknum != NF_INET_POST_ROUTING);

	ct = nf_ct_get(skb, &ctinfo);

	WARN_ON(!(ct && (ctinfo == IP_CT_NEW || ctinfo == IP_CT_RELATED ||
			 ctinfo == IP_CT_RELATED_REPLY)));

	/* Source address is 0.0.0.0 - locally generated packet that is
	 * probably not supposed to be masqueraded.
	 */
	if (ct->tuplehash[IP_CT_DIR_ORIGINAL].tuple.src.u3.ip == 0)
		return NF_ACCEPT;

	rt = skb_rtable(skb);
	nh = rt_nexthop(rt, ip_hdr(skb)->daddr);
	newsrc = inet_select_addr(out, nh, RT_SCOPE_UNIVERSE);
	if (!newsrc) {
		pr_info("%s ate my IP address\n", out->name);
		return NF_DROP;
	}

	nat = nf_ct_nat_ext_add(ct);
	if (nat)
		nat->masq_index = out->ifindex;

/* RFC 4787 - 4.2.2.  Port Parity
   i.e., an even port will be mapped to an even port, and an odd port will be mapped to an odd port.*/

#define CHECK_PORT_PARITY(a, b) ((a%2)==(b%2))
       if (READ_ONCE(lg6851f_nat_mode) == 1
           && (nfct_help(ct) == NULL || rcu_access_pointer(nfct_help(ct)->helper) == NULL)
           && nf_ct_protonum(ct) == IPPROTO_UDP) {
               unsigned int ret;
               u_int16_t minport;
               u_int16_t maxport;
               struct nf_conntrack_expect *exp;


               /* Choose port */
               spin_lock_bh(&nf_conntrack_expect_lock);
               /* Look for existing expectation */
               exp = find_fullcone_exp(ct, newsrc);
               if (exp) {
                       minport = maxport = exp->tuple.dst.u.udp.port;

               } else { /* no previous expect */
                       u32 newport, tmpport, orgport;
                       bool found_port = false;

                       minport = range->min_proto.all == 0?
                               ct->tuplehash[IP_CT_DIR_ORIGINAL].tuple.src.
                               u.udp.port : range->min_proto.all;
                       maxport = range->max_proto.all == 0?
                               htons(65535) : range->max_proto.all;
                        orgport = ntohs(minport);
                       for (newport = ntohs(minport),tmpport = ntohs(maxport);
                            newport <= tmpport; newport++) {
                               if (CHECK_PORT_PARITY(orgport, newport) && !find_exp(newsrc, htons(newport), ct)) {

                                       minport = maxport = htons(newport);
                                       found_port = true;
                                       break;
                               }
                       }
                       if (!found_port) {
                               spin_unlock_bh(&nf_conntrack_expect_lock);
                               return NF_DROP;
                       }
               }
               spin_unlock_bh(&nf_conntrack_expect_lock);


       memset(&newrange.min_addr, 0, sizeof(newrange.min_addr));
       memset(&newrange.max_addr, 0, sizeof(newrange.max_addr));

               newrange.flags = range->flags | NF_NAT_RANGE_MAP_IPS |
                       NF_NAT_RANGE_PROTO_SPECIFIED;
               newrange.max_addr.ip = newrange.min_addr.ip = newsrc;
               newrange.min_proto.udp.port = newrange.max_proto.udp.port = minport;

               /* Set ct helper */
               ret = nf_nat_setup_info(ct, &newrange, NF_NAT_MANIP_SRC);
               if (ret == NF_ACCEPT) {
                       struct nf_conn_help *help = nfct_help(ct);
                       if (help == NULL)
                               help = nf_ct_helper_ext_add(ct, GFP_ATOMIC);
                       if (help != NULL) {
                               help->helper = &nf_conntrack_helper_bcm_nat;

                       }
               }
               return ret;
       }

	/* Transfer from original range. */
	memset(&newrange.min_addr, 0, sizeof(newrange.min_addr));
	memset(&newrange.max_addr, 0, sizeof(newrange.max_addr));
	newrange.flags       = range->flags | NF_NAT_RANGE_MAP_IPS;
	if (READ_ONCE(lg6851f_nat_mode) == 2)
		newrange.flags |= NF_NAT_RANGE_PROTO_RANDOM_FULLY;
	newrange.min_addr.ip = newsrc;
	newrange.max_addr.ip = newsrc;
	newrange.min_proto   = range->min_proto;
	newrange.max_proto   = range->max_proto;

	/* Hand modified range to generic setup. */
	return nf_nat_setup_info(ct, &newrange, NF_NAT_MANIP_SRC);
}

static unsigned checks;
#define CHECK(x) do {checks++;assert(x);} while(0)
static void init(struct nf_conn *ct){memset(ct,0,sizeof(*ct));ct->net=1;ct->zone=2;ct->tuplehash[0].tuple.src.u3.ip=htonl(0x0a000001);ct->tuplehash[0].tuple.src.u.udp.port=htons(40000);ct->tuplehash[0].tuple.dst.protonum=IPPROTO_UDP;selected_ip=htonl(0xc0000201);memset(occupied,0,sizeof(occupied));memset(buckets,0,sizeof(buckets));memset(&captured,0,sizeof(captured));probes=setups=0;}
int main(void){
 struct nf_conn ct,other;struct sk_buff skb={.ct=&ct};struct net_device dev={.ifindex=7,.name="mock0"};struct nf_nat_range2 range={.flags=0x100};
 init(&ct);lg6851f_nat_mode=0;CHECK(nf_nat_masquerade_ipv4(&skb,4,&range,&dev)==NF_ACCEPT);CHECK(captured.flags==(0x100|NF_NAT_RANGE_MAP_IPS));CHECK(!ct.hashelp);CHECK(ct.nat.masq_index==7);CHECK(probes==0);
 init(&ct);lg6851f_nat_mode=2;nf_nat_masquerade_ipv4(&skb,4,&range,&dev);CHECK(captured.flags&(NF_NAT_RANGE_PROTO_RANDOM_FULLY));CHECK(!ct.hashelp);CHECK(probes==0);
 init(&ct);lg6851f_nat_mode=1;nf_nat_masquerade_ipv4(&skb,4,&range,&dev);CHECK(captured.min_proto.udp.port==htons(40000));CHECK(captured.min_proto.all==captured.max_proto.all);CHECK(ct.hashelp&&ct.help.helper==&nf_conntrack_helper_bcm_nat);CHECK(captured.min_addr.ip==selected_ip);
 init(&ct);occupied[40000]=1;nf_nat_masquerade_ipv4(&skb,4,&range,&dev);CHECK(captured.min_proto.all==htons(40002));CHECK(probes<=3);
 init(&ct);ct.tuplehash[0].tuple.src.u.udp.port=htons(65535);occupied[65535]=1;CHECK(nf_nat_masquerade_ipv4(&skb,4,&range,&dev)==NF_DROP);CHECK(probes==1);CHECK(setups==0);
 init(&ct);memset(occupied,1,sizeof(occupied));CHECK(nf_nat_masquerade_ipv4(&skb,4,&range,&dev)==NF_DROP);CHECK(probes<=65536);CHECK(setups==0);
 init(&ct);ct.tuplehash[0].tuple.dst.protonum=IPPROTO_TCP;nf_nat_masquerade_ipv4(&skb,4,&range,&dev);CHECK(!ct.hashelp);CHECK(probes==0);CHECK(captured.flags==(0x100|NF_NAT_RANGE_MAP_IPS));
 init(&ct);selected_ip=0;CHECK(nf_nat_masquerade_ipv4(&skb,4,&range,&dev)==NF_DROP);CHECK(setups==0);
 init(&ct);ct.tuplehash[0].tuple.src.u3.ip=0;CHECK(nf_nat_masquerade_ipv4(&skb,4,&range,&dev)==NF_ACCEPT);CHECK(setups==0);
 init(&ct);other=ct;struct nf_conntrack_expect exp={.master=&other,.expectfn=bcm_nat_expect};exp.saved_addr=ct.tuplehash[0].tuple.src.u3;exp.saved_proto=ct.tuplehash[0].tuple.src.u;exp.tuple.dst.protonum=IPPROTO_UDP;exp.tuple.dst.u3.ip=selected_ip;exp.tuple.dst.u.udp.port=htons(50000);buckets[0].first=&exp;
 CHECK(find_fullcone_exp(&ct,selected_ip)==&exp);other.net=3;CHECK(!find_fullcone_exp(&ct,selected_ip));other.net=ct.net;other.zone=9;CHECK(!find_fullcone_exp(&ct,selected_ip));other.zone=ct.zone;CHECK(!find_fullcone_exp(&ct,htonl(0xc0000202)));exp.saved_proto.all=htons(40001);CHECK(!find_fullcone_exp(&ct,selected_ip));exp.saved_proto=ct.tuplehash[0].tuple.src.u;
 nf_nat_masquerade_ipv4(&skb,4,&range,&dev);CHECK(captured.min_proto.all==htons(50000));CHECK(probes==0);
 struct kernel_param kp={.arg=&lg6851f_nat_mode};for(int i=0;i<3;i++){char value[4];snprintf(value,sizeof(value),"%d",i);CHECK(!lg6851f_set_nat_mode(value,&kp));CHECK(lg6851f_nat_mode==(unsigned)i);}CHECK(lg6851f_set_nat_mode("3",&kp)<0);CHECK(lg6851f_set_nat_mode("-1",&kp)<0);CHECK(lg6851f_set_nat_mode("1;id",&kp)<0);CHECK(lg6851f_nat_mode==2);
 for(int p=1;p<=65535;p+=61){init(&ct);lg6851f_nat_mode=1;ct.tuplehash[0].tuple.src.u.udp.port=htons(p);occupied[p]=1;int result=nf_nat_masquerade_ipv4(&skb,4,&range,&dev);if(p+2<=65535){CHECK(result==NF_ACCEPT);CHECK(ntohs(captured.min_proto.all)==p+2);}else CHECK(result==NF_DROP);CHECK(probes<=3);}
 printf("{\"passed\":true,\"assertions\":%u,\"actual_production_functions\":true,\"legacy_and_symmetric_flags\":true,\"namespace_zone_external_address_isolation\":true,\"port_exhaustion_bounded\":true,\"real_packet_NAT_tested\":false}\n",checks);
}
