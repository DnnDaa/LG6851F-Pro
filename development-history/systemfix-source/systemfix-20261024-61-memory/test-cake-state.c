
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>
#include <errno.h>
typedef uint8_t u8;typedef uint16_t u16;typedef uint32_t u32;typedef uint64_t u64;
typedef int16_t s16;typedef int32_t s32;typedef int64_t ktime_t;
#define CAKE_MAX_TINS 8
#define CAKE_QUEUES 1024
#define GFP_KERNEL 0
#define CAKE_DIFFSERV_BESTEFFORT 3
#define CAKE_DIFFSERV_DIFFSERV3 0
#define CAKE_FLOW_TRIPLE 7
#define CAKE_FLAG_SPLIT_GSO 16
#define CAKE_FLAG_INGRESS 1
#define NSEC_PER_USEC 1000
#define SKB_DROP_REASON_QUEUE_PURGE 0
#define SKB_DROP_REASON_QDISC_OVERLIMIT 1
#define WRITE_ONCE(a,b) ((a)=(b))
#define unlikely(a) (a)
#define NL_SET_ERR_MSG(a,b) ((void)0)
struct list_head{struct list_head *next,*prev;};
#define INIT_LIST_HEAD(p) do{(p)->next=(p);(p)->prev=(p);}while(0)
struct sk_buff{struct sk_buff *next;u32 len,truesize;};
struct cobalt_params {
	u64	interval;
	u64	target;
	u64	mtu_time;
	u32	p_inc;
	u32	p_dec;
};
struct cobalt_vars {
	u32	count;
	u32	rec_inv_sqrt;
	ktime_t	drop_next;
	ktime_t	blue_timer;
	u32     p_drop;
	bool	dropping;
	bool    ecn_marked;
};
struct cake_flow {
	/* this stuff is all needed per-flow at dequeue time */
	struct sk_buff	  *head;
	struct sk_buff	  *tail;
	struct list_head  flowchain;
	s32		  deficit;
	u32		  dropped;
	struct cobalt_vars cvars;
	u16		  srchost; /* index into cake_host table */
	u16		  dsthost;
	u8		  set;
};
struct cake_host {
	u32 srchost_tag;
	u32 dsthost_tag;
	u16 srchost_bulk_flow_count;
	u16 dsthost_bulk_flow_count;
};
struct cake_heap_entry {
	u16 t:3, b:10;
};
struct cake_tin_data {
	struct cake_flow flows[CAKE_QUEUES];
	u32	backlogs[CAKE_QUEUES];
	u32	tags[CAKE_QUEUES]; /* for set association */
	u16	overflow_idx[CAKE_QUEUES];
	struct cake_host hosts[CAKE_QUEUES]; /* for triple isolation */
	u16	flow_quantum;

	struct cobalt_params cparams;
	u32	drop_overlimit;
	u16	bulk_flow_count;
	u16	sparse_flow_count;
	u16	decaying_flow_count;
	u16	unresponsive_flow_count;

	u32	max_skblen;

	struct list_head new_flows;
	struct list_head old_flows;
	struct list_head decaying_flows;

	/* time_next = time_this + ((len * rate_ns) >> rate_shft) */
	ktime_t	time_next_packet;
	u64	tin_rate_ns;
	u64	tin_rate_bps;
	u16	tin_rate_shft;

	u16	tin_quantum;
	s32	tin_deficit;
	u32	tin_backlog;
	u32	tin_dropped;
	u32	tin_ecn_mark;

	u32	packets;
	u64	bytes;

	u32	ack_drops;

	/* moving averages */
	u64 avge_delay;
	u64 peak_delay;
	u64 base_delay;

	/* hash function stats */
	u32	way_directs;
	u32	way_hits;
	u32	way_misses;
	u32	way_collisions;
	u8	tin_index;
};
struct cake_sched_config {
	u64		rate_bps;
	u64		interval;
	u64		target;
	u64		sync_time;
	u32		buffer_config_limit;
	u32		fwmark_mask;
	u16		fwmark_shft;
	s16		rate_overhead;
	u16		rate_mpu;
	u16		rate_flags;
	u8		tin_mode;
	u8		flow_mode;
	u8		atm_mode;
	u8		ack_filter;
	u8		is_shared;
};

struct cake_sched_data{
 struct cake_tin_data *tins[8];
 struct cake_heap_entry overflow_heap[8192];
 struct cake_sched_config *config,initial_config;
 void *block,*filter_list;
 u32 max_netlen,max_adjlen,min_netlen,min_adjlen,active_queues;
 u64 avg_peak_bandwidth,last_checked_active;
 u32 cur_tin,cur_flow,buffer_used;u16 overflow_timeout,tin_cnt;
 int watchdog;
};
struct Qdisc{struct cake_sched_data qd;u32 limit;struct{u32 qlen;}q;struct{u32 backlog;}qstats;};
struct nlattr{int mode;bool overhead;bool bad;};
struct netlink_ext_ack{};
static unsigned long checks=0;
#define CHECK(x) do{if(!(x)){fprintf(stderr,"FAIL line %d: %s\n",__LINE__,#x);abort();}checks++;}while(0)
static int lock_depth,alloc_live,alloc_budget=-1,alloc_calls,packets_live;
static void *kvzalloc(size_t n,int flags){
 CHECK(!lock_depth);alloc_calls++;
 if(alloc_budget==0)return NULL;if(alloc_budget>0)alloc_budget--;
 void*p=calloc(1,n);CHECK(p);alloc_live++;return p;
}
static void kvfree(void*p){if(p){CHECK(!lock_depth);free(p);alloc_live--;}}
static struct cake_sched_data *qdisc_priv(struct Qdisc*s){return &s->qd;}
static void sch_tree_lock(struct Qdisc*s){CHECK(lock_depth==0);lock_depth++;}
static void sch_tree_unlock(struct Qdisc*s){CHECK(lock_depth==1);lock_depth--;}
static void qdisc_watchdog_init(int*w,struct Qdisc*s){*w=1;}
static void qdisc_watchdog_cancel(int*w){*w=0;}
static int tcf_block_get(void**b,void**l,struct Qdisc*s,void*e){*b=(void*)1;return 0;}
static void tcf_block_put(void*b){}
static ktime_t ktime_get(void){return 0;}
static void cobalt_vars_init(struct cobalt_vars*v){memset(v,0,sizeof(*v));v->rec_inv_sqrt=0xffffffff;}
static bool cobalt_queue_full(struct cobalt_vars*v,struct cobalt_params*p,ktime_t t){return false;}
static u32 qdisc_pkt_len(struct sk_buff*s){return s->len;}
static struct sk_buff *dequeue_head(struct cake_flow*f){struct sk_buff*s=f->head;if(s){f->head=s->next;if(!f->head)f->tail=NULL;}return s;}
static void kfree_skb_reason(struct sk_buff*s,int why){CHECK(packets_live>0);packets_live--;free(s);}
static void qdisc_drop_reason(struct sk_buff*s,struct Qdisc*q,struct sk_buff**to_free,int why){kfree_skb_reason(s,why);}
static int cake_advance_shaper(struct cake_sched_data*q,struct cake_tin_data*b,struct sk_buff*s,ktime_t t,bool drop){return s->len;}
static int cake_config_change(struct cake_sched_config*q,struct nlattr*opt,struct netlink_ext_ack*e,bool*changed){
 if(opt->bad)return -EINVAL;q->tin_mode=opt->mode;*changed=opt->overhead;return 0;
}
static void cake_reconfigure(struct Qdisc*s){
 struct cake_sched_data*q=&s->qd;int mode=q->config->tin_mode;
 q->tin_cnt=mode==3?1:mode==0?3:mode==1?4:8;
 for(int i=0;i<q->tin_cnt;i++)CHECK(q->tins[i]);
}
static u16 quantum_div[1025];
static void cake_heap_swap(struct cake_sched_data *q, u16 i, u16 j)
{
	struct cake_heap_entry ii = q->overflow_heap[i];
	struct cake_heap_entry jj = q->overflow_heap[j];

	q->overflow_heap[i] = jj;
	q->overflow_heap[j] = ii;

	if (q->tins[ii.t])
		q->tins[ii.t]->overflow_idx[ii.b] = j;
	if (q->tins[jj.t])
		q->tins[jj.t]->overflow_idx[jj.b] = i;
}
static u32 cake_heap_get_backlog(const struct cake_sched_data *q, u16 i)
{
	struct cake_heap_entry ii = q->overflow_heap[i];

	return q->tins[ii.t] ? q->tins[ii.t]->backlogs[ii.b] : 0;
}
static void cake_heapify(struct cake_sched_data *q, u16 i)
{
	static const u32 a = CAKE_MAX_TINS * CAKE_QUEUES;
	u32 mb = cake_heap_get_backlog(q, i);
	u32 m = i;

	while (m < a) {
		u32 l = m + m + 1;
		u32 r = l + 1;

		if (l < a) {
			u32 lb = cake_heap_get_backlog(q, l);

			if (lb > mb) {
				m  = l;
				mb = lb;
			}
		}

		if (r < a) {
			u32 rb = cake_heap_get_backlog(q, r);

			if (rb > mb) {
				m  = r;
				mb = rb;
			}
		}

		if (m != i) {
			cake_heap_swap(q, i, m);
			i = m;
		} else {
			break;
		}
	}
}
static void cake_heapify_up(struct cake_sched_data *q, u16 i)
{
	while (i > 0 && i < CAKE_MAX_TINS * CAKE_QUEUES) {
		u16 p = (i - 1) >> 1;
		u32 ib = cake_heap_get_backlog(q, i);
		u32 pb = cake_heap_get_backlog(q, p);

		if (ib > pb) {
			cake_heap_swap(q, i, p);
			i = p;
		} else {
			break;
		}
	}
}
static unsigned int cake_drop(struct Qdisc *sch, struct sk_buff **to_free)
{
	struct cake_sched_data *q = qdisc_priv(sch);
	ktime_t now = ktime_get();
	u32 idx = 0, tin = 0, len;
	struct cake_heap_entry qq;
	struct cake_tin_data *b;
	struct cake_flow *flow;
	struct sk_buff *skb;

	if (!q->overflow_timeout) {
		int i;
		/* Build fresh max-heap */
		for (i = CAKE_MAX_TINS * CAKE_QUEUES / 2 - 1; i >= 0; i--)
			cake_heapify(q, i);
	}
	q->overflow_timeout = 65535;

	/* select longest queue for pruning */
	qq  = q->overflow_heap[0];
	tin = qq.t;
	idx = qq.b;

	b = q->tins[tin];
	if (unlikely(!b)) {
		q->overflow_timeout = 0;
		return idx + (tin << 16);
	}
	flow = &b->flows[idx];
	skb = dequeue_head(flow);
	if (unlikely(!skb)) {
		/* heap has gone wrong, rebuild it next time */
		q->overflow_timeout = 0;
		return idx + (tin << 16);
	}

	if (cobalt_queue_full(&flow->cvars, &b->cparams, now))
		b->unresponsive_flow_count++;

	len = qdisc_pkt_len(skb);
	q->buffer_used      -= skb->truesize;
	b->backlogs[idx]    -= len;
	b->tin_backlog      -= len;
	sch->qstats.backlog -= len;

	flow->dropped++;
	WRITE_ONCE(b->tin_dropped, b->tin_dropped + 1);

	if (q->config->rate_flags & CAKE_FLAG_INGRESS)
		cake_advance_shaper(q, b, skb, now, true);

	qdisc_drop_reason(skb, sch, to_free, SKB_DROP_REASON_QDISC_OVERLIMIT);
	sch->q.qlen--;

	cake_heapify(q, 0);

	return idx + (tin << 16);
}
static struct sk_buff *cake_dequeue_one(struct Qdisc *sch)
{
	struct cake_sched_data *q = qdisc_priv(sch);
	struct cake_tin_data *b = q->tins[q->cur_tin];
	struct cake_flow *flow = &b->flows[q->cur_flow];
	struct sk_buff *skb = NULL;
	u32 len;

	if (flow->head) {
		skb = dequeue_head(flow);
		len = qdisc_pkt_len(skb);
		b->backlogs[q->cur_flow] -= len;
		b->tin_backlog		 -= len;
		sch->qstats.backlog      -= len;
		q->buffer_used		 -= skb->truesize;
		sch->q.qlen--;

		if (q->overflow_timeout)
			cake_heapify(q, b->overflow_idx[q->cur_flow]);
	}
	return skb;
}
static void cake_clear_tin(struct Qdisc *sch, u16 tin)
{
	struct cake_sched_data *q = qdisc_priv(sch);
	struct sk_buff *skb;

	if (!q->tins[tin])
		return;

	q->cur_tin = tin;
	for (q->cur_flow = 0; q->cur_flow < CAKE_QUEUES; q->cur_flow++)
		while (!!(skb = cake_dequeue_one(sch)))
			kfree_skb_reason(skb, SKB_DROP_REASON_QUEUE_PURGE);
}
static void cake_reset(struct Qdisc *sch)
{
	struct cake_sched_data *q = qdisc_priv(sch);
	u32 c;

	if (!q->tins[0])
		return;

	for (c = 0; c < CAKE_MAX_TINS; c++)
		cake_clear_tin(sch, c);
}
static struct cake_tin_data *cake_alloc_tin(u8 index)
{
	struct cake_tin_data *b;
	int j;

	b = kvzalloc(sizeof(*b), GFP_KERNEL);
	if (!b)
		return NULL;
	b->tin_index = index;
	INIT_LIST_HEAD(&b->new_flows);
	INIT_LIST_HEAD(&b->old_flows);
	INIT_LIST_HEAD(&b->decaying_flows);
	for (j = 0; j < CAKE_QUEUES; j++) {
		INIT_LIST_HEAD(&b->flows[j].flowchain);
		cobalt_vars_init(&b->flows[j].cvars);
	}
	return b;
}
static int cake_grow_tins(struct Qdisc *sch)
{
	struct cake_sched_data *q = qdisc_priv(sch);
	struct cake_tin_data *fresh[CAKE_MAX_TINS] = {};
	int i, k;
	bool changed = false;

	for (i = 0; i < CAKE_MAX_TINS; i++) {
		if (q->tins[i])
			continue;
		fresh[i] = cake_alloc_tin(i);
		if (!fresh[i])
			goto nomem;
		changed = true;
	}
	if (!changed)
		return 0;
	sch_tree_lock(sch);
	for (i = 0; i < CAKE_MAX_TINS; i++)
		if (fresh[i])
			q->tins[i] = fresh[i];
	/* Zero entries may already have moved in the overflow heap. Rebuild
	 * only their reverse indexes; leave existing flows and queue state intact.
	 */
	for (k = 0; k < CAKE_QUEUES * CAKE_MAX_TINS; k++) {
		struct cake_heap_entry e = q->overflow_heap[k];

		if (fresh[e.t])
			fresh[e.t]->overflow_idx[e.b] = k;
	}
	sch_tree_unlock(sch);
	return 0;

nomem:
	for (i = 0; i < CAKE_MAX_TINS; i++)
		kvfree(fresh[i]);
	return -ENOMEM;
}
static int cake_change(struct Qdisc *sch, struct nlattr *opt,
		       struct netlink_ext_ack *extack)
{
	struct cake_sched_data *qd = qdisc_priv(sch);
	struct cake_sched_config *q = qd->config;
	struct cake_sched_config candidate = *q;
	bool overhead_changed = false;
	int ret;

	if (q->is_shared) {
		NL_SET_ERR_MSG(extack, "can't reconfigure cake_mq sub-qdiscs");
		return -EOPNOTSUPP;
	}
	ret = cake_config_change(&candidate, opt, extack, &overhead_changed);
	if (ret)
		return ret;
	if (qd->tins[0] && candidate.tin_mode != CAKE_DIFFSERV_BESTEFFORT) {
		ret = cake_grow_tins(sch);
		if (ret) {
			NL_SET_ERR_MSG(extack, "cannot allocate CAKE Diffserv state");
			return ret;
		}
	}
	if (qd->tins[0])
		sch_tree_lock(sch);
	*q = candidate;
	if (overhead_changed) {
		qd->max_netlen = 0;
		qd->max_adjlen = 0;
		qd->min_netlen = ~0;
		qd->min_adjlen = ~0;
	}
	if (qd->tins[0]) {
		cake_reconfigure(sch);
		sch_tree_unlock(sch);
	}
	return 0;
}
static void cake_destroy(struct Qdisc *sch)
{
	struct cake_sched_data *q = qdisc_priv(sch);
	int i;

	qdisc_watchdog_cancel(&q->watchdog);
	tcf_block_put(q->block);
	for (i = 0; i < CAKE_MAX_TINS; i++)
		kvfree(q->tins[i]);
}
static void cake_config_init(struct cake_sched_config *q, bool is_shared)
{
	q->tin_mode = CAKE_DIFFSERV_DIFFSERV3;
	q->flow_mode  = CAKE_FLOW_TRIPLE;

	q->rate_bps = 0; /* unlimited by default */

	q->interval = 100000; /* 100ms default */
	q->target   =   5000; /* 5ms: codel RFC argues
			       * for 5 to 10% of interval
			       */
	q->rate_flags |= CAKE_FLAG_SPLIT_GSO;
	q->is_shared = is_shared;
	q->sync_time = 200 * NSEC_PER_USEC;
}
static int cake_init(struct Qdisc *sch, struct nlattr *opt,
		     struct netlink_ext_ack *extack)
{
	struct cake_sched_data *qd = qdisc_priv(sch);
	struct cake_sched_config *q = &qd->initial_config;
	int i, j, err;

	cake_config_init(q, false);

	sch->limit = 10240;
	qd->cur_tin = 0;
	qd->cur_flow  = 0;
	qd->config = q;

	qdisc_watchdog_init(&qd->watchdog, sch);

	if (opt) {
		err = cake_change(sch, opt, extack);
		if (err)
			return err;
	}

	err = tcf_block_get(&qd->block, &qd->filter_list, sch, extack);
	if (err)
		return err;

	quantum_div[0] = ~0;
	for (i = 1; i <= CAKE_QUEUES; i++)
		quantum_div[i] = 65535 / i;

	/* Existing default/shared Diffserv users retain all eight tins. */
	for (i = 0; i < CAKE_MAX_TINS; i++) {
		if (i == 0 || q->tin_mode != CAKE_DIFFSERV_BESTEFFORT) {
			qd->tins[i] = cake_alloc_tin(i);
			if (!qd->tins[i])
				return -ENOMEM;
		}
		for (j = 0; j < CAKE_QUEUES; j++) {
			u32 k = j * CAKE_MAX_TINS + i;

			qd->overflow_heap[k].t = i;
			qd->overflow_heap[k].b = j;
			if (qd->tins[i])
				qd->tins[i]->overflow_idx[j] = k;
		}
	}

	cake_reconfigure(sch);
	qd->avg_peak_bandwidth = q->rate_bps;
	qd->min_netlen = ~0;
	qd->min_adjlen = ~0;
	qd->active_queues = 0;
	qd->last_checked_active = 0;

	return 0;
}
static void cake_config_replace(struct Qdisc *sch, struct cake_sched_config *cfg)
{
	struct cake_sched_data *qd = qdisc_priv(sch);

	qd->config = cfg;
	cake_reconfigure(sch);
}
struct legacy_sched_data { struct cake_tin_data tins[8]; struct cake_heap_entry overflow_heap[8192]; u16 overflow_timeout; };

static void legacy_cake_heap_swap(struct legacy_sched_data *q, u16 i, u16 j)
{
	struct cake_heap_entry ii = q->overflow_heap[i];
	struct cake_heap_entry jj = q->overflow_heap[j];

	q->overflow_heap[i] = jj;
	q->overflow_heap[j] = ii;

	q->tins[ii.t].overflow_idx[ii.b] = j;
	q->tins[jj.t].overflow_idx[jj.b] = i;
}

static u32 legacy_cake_heap_get_backlog(const struct legacy_sched_data *q, u16 i)
{
	struct cake_heap_entry ii = q->overflow_heap[i];

	return q->tins[ii.t].backlogs[ii.b];
}

static void legacy_cake_heapify(struct legacy_sched_data *q, u16 i)
{
	static const u32 a = CAKE_MAX_TINS * CAKE_QUEUES;
	u32 mb = legacy_cake_heap_get_backlog(q, i);
	u32 m = i;

	while (m < a) {
		u32 l = m + m + 1;
		u32 r = l + 1;

		if (l < a) {
			u32 lb = legacy_cake_heap_get_backlog(q, l);

			if (lb > mb) {
				m  = l;
				mb = lb;
			}
		}

		if (r < a) {
			u32 rb = legacy_cake_heap_get_backlog(q, r);

			if (rb > mb) {
				m  = r;
				mb = rb;
			}
		}

		if (m != i) {
			legacy_cake_heap_swap(q, i, m);
			i = m;
		} else {
			break;
		}
	}
}

static void legacy_cake_heapify_up(struct legacy_sched_data *q, u16 i)
{
	while (i > 0 && i < CAKE_MAX_TINS * CAKE_QUEUES) {
		u16 p = (i - 1) >> 1;
		u32 ib = legacy_cake_heap_get_backlog(q, i);
		u32 pb = legacy_cake_heap_get_backlog(q, p);

		if (ib > pb) {
			legacy_cake_heap_swap(q, i, p);
			i = p;
		} else {
			break;
		}
	}
}


static struct legacy_sched_data *oracle;
static void verify(struct Qdisc*s,bool ordered){
 struct cake_sched_data*q=&s->qd;
 for(int k=0;k<8192;k++){
  struct cake_heap_entry e=q->overflow_heap[k];CHECK(e.t<8&&e.b<1024);
  if(q->tins[e.t])CHECK(q->tins[e.t]->overflow_idx[e.b]==k);
 }
 if(oracle){CHECK(!memcmp(q->overflow_heap,oracle->overflow_heap,sizeof(q->overflow_heap)));for(int t=0;t<8;t++)if(q->tins[t])CHECK(!memcmp(q->tins[t]->backlogs,oracle->tins[t].backlogs,sizeof(oracle->tins[t].backlogs)));}
}
static struct Qdisc *make(int mode){
 struct Qdisc*s=calloc(1,sizeof(*s));struct nlattr o={.mode=mode};
 CHECK(cake_init(s,&o,NULL)==0);return s;
}
static void finish(struct Qdisc*s){cake_reset(s);CHECK(!s->q.qlen&&!s->qstats.backlog&&!s->qd.buffer_used);cake_destroy(s);free(s);CHECK(alloc_live==0&&lock_depth==0&&packets_live==0);}
static uint32_t rng=12345678;
static uint32_t rnd(void){rng^=rng<<13;rng^=rng>>17;rng^=rng<<5;return rng;}
static void add(struct Qdisc*s,int t,int f,u32 n){
 struct cake_sched_data*q=&s->qd;struct cake_tin_data*b=q->tins[t];CHECK(b);
 struct sk_buff*p=calloc(1,sizeof(*p));p->len=n;p->truesize=n+256;packets_live++;
 if(b->flows[f].tail)b->flows[f].tail->next=p;else b->flows[f].head=p;b->flows[f].tail=p;
 b->backlogs[f]+=n;b->tin_backlog+=n;s->q.qlen++;s->qstats.backlog+=n;q->buffer_used+=p->truesize;
 cake_heapify_up(q,b->overflow_idx[f]);
 if(oracle){oracle->tins[t].backlogs[f]+=n;legacy_cake_heapify_up(oracle,oracle->tins[t].overflow_idx[f]);}
}
static void drop_one(struct Qdisc*s){
 u32 expected=0;
 if(oracle){
  if(!oracle->overflow_timeout)for(int i=4095;i>=0;i--)legacy_cake_heapify(oracle,i);
  struct cake_heap_entry e=oracle->overflow_heap[0];struct cake_tin_data*b=s->qd.tins[e.t];expected=e.b+(e.t<<16);
  // The original heap deliberately requests a rebuild if its chosen flow is
  // empty. Preserve that behavior instead of imposing a stronger heap invariant.
  if(b&&b->flows[e.b].head){oracle->tins[e.t].backlogs[e.b]-=b->flows[e.b].head->len;legacy_cake_heapify(oracle,0);oracle->overflow_timeout=65535;}
  else oracle->overflow_timeout=0;
 }
 struct sk_buff*z=NULL;u32 actual=cake_drop(s,&z);if(oracle)CHECK(actual==expected&&s->qd.overflow_timeout==oracle->overflow_timeout);
}
int main(void){
 struct Qdisc*s=make(3);CHECK(alloc_live==1);CHECK(s->limit==10240);
 oracle=calloc(1,sizeof(*oracle));for(int t=0;t<8;t++)for(int f=0;f<1024;f++){int k=f*8+t;oracle->overflow_heap[k].t=t;oracle->overflow_heap[k].b=f;oracle->tins[t].overflow_idx[f]=k;}
 for(int i=1;i<8;i++)CHECK(!s->qd.tins[i]);
 for(int f=0;f<1024;f++){struct cake_flow*a=&s->qd.tins[0]->flows[f];CHECK(a->flowchain.next==&a->flowchain&&a->flowchain.prev==&a->flowchain);}
 // Every native flow remains usable in the one active tin.
 for(int i=0;i<10000;i++){int f=rnd()%1024;add(s,0,f,1+rnd()%1400);if(i%7==0)drop_one(s);if(i%1000==0)verify(s,true);}
 add(s,0,0,600);struct cake_tin_data*first=s->qd.tins[0];struct sk_buff*held=first->flows[0].head;
 struct nlattr mode8={.mode=2,.overhead=true};CHECK(cake_change(s,&mode8,NULL)==0);CHECK(alloc_live==8&&s->qd.tins[0]==first&&first->flows[0].head==held);verify(s,true);
 // Expanding after zero heap entries moved must rebuild the reverse map.
 for(int i=0;i<16000;i++){add(s,rnd()%8,rnd()%1024,1+rnd()%1400);if(i%3==0)drop_one(s);if(i%2000==0)verify(s,true);}
 int prior=alloc_calls;CHECK(cake_change(s,&mode8,NULL)==0&&alloc_calls==prior);verify(s,true);free(oracle);oracle=NULL;finish(s);
 // Each staged allocation failure leaves both live packets and settings intact.
 for(int fail=0;fail<7;fail++){
  s=make(3);add(s,0,10,1200);struct cake_tin_data*old=s->qd.tins[0];struct sk_buff*packet=old->flows[10].head;s->qd.max_netlen=123;
  alloc_budget=fail;CHECK(cake_change(s,&mode8,NULL)==-ENOMEM);alloc_budget=-1;
  CHECK(s->qd.config->tin_mode==3&&s->qd.max_netlen==123&&s->qd.tins[0]==old&&old->flows[10].head==packet&&s->q.qlen==1&&alloc_live==1);
  verify(s,true);finish(s);
 }
 // Partial creation is safe for the kernel's reset/destroy error cleanup.
 for(int fail=0;fail<8;fail++){
  s=calloc(1,sizeof(*s));struct nlattr o={.mode=2};alloc_budget=fail;
  CHECK(cake_init(s,&o,NULL)==-ENOMEM);alloc_budget=-1;finish(s);
 }
 // All Diffserv modes and default/shared cake_mq children retain eight tins.
 for(int mode=0;mode<5;mode++){
  if(mode==3)continue;s=make(mode);CHECK(alloc_live==8);
  for(int t=0;t<s->qd.tin_cnt;t++)add(s,t,100+t,700+t);
  struct cake_sched_config cfg=s->qd.initial_config;cfg.is_shared=true;
  for(int next=0;next<5;next++){cfg.tin_mode=next;cake_config_replace(s,&cfg);CHECK(alloc_live==8);}
  CHECK(cake_change(s,&mode8,NULL)==-EOPNOTSUPP);finish(s);
 }
 // Switching back retains allocated packets/addresses; reset frees everything.
 s=make(3);CHECK(cake_change(s,&mode8,NULL)==0);for(int t=0;t<8;t++)add(s,t,t,1000+t);
 struct nlattr be={.mode=3};CHECK(cake_change(s,&be,NULL)==0&&alloc_live==8);finish(s);
 s=make(3);struct nlattr bad={.bad=true};CHECK(cake_change(s,&bad,NULL)==-EINVAL&&s->qd.config->tin_mode==3&&alloc_live==1);finish(s);
 printf("{\"passed\":true,\"assertions\":%lu,\"packet_operations\":26000,\"flow_queues_per_tin\":1024,\"max_tins\":8,\"new_tin_bytes\":%zu,\"scheduler_bytes\":%zu,\"fault_points\":15,\"allocation_under_spinlock\":false,\"hardware_test\":false}\n",checks,sizeof(struct cake_tin_data),sizeof(struct cake_sched_data));
 return 0;
}
