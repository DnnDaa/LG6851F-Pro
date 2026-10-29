
#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
typedef uint32_t u32;typedef uint64_t u64;
#define __iomem
#define BIT(n) (1U<<(n))
#define ARRAY_SIZE(x) (sizeof(x)/sizeof((x)[0]))
#define HZ 100UL
static unsigned long jiffies=100;
#define time_before(a,b) ((long)((a)-(b))<0)
typedef int spinlock_t;
#define __SPIN_LOCK_UNLOCKED(x) 0
#define spin_trylock_irqsave(p,f) ((f)=0,*(p)?0:(*(p)=1))
#define spin_unlock_irqrestore(p,f) do { (void)(f);*(p)=0; } while(0)
typedef struct {long long value;} atomic64_t;
#define atomic64_inc(p) (++(p)->value)
#define atomic64_read(p) ((p)->value)
struct seq_file {int unused;};
#define seq_printf(...) ((void)0)
static unsigned regs[2][5],mode[2],pending[2],reads[2],writes[2],warnings;
#define MIB_SER_CR ((void*)&regs[0][0])
#define MIB_SER_R0 ((void*)&regs[0][1])
#define MIB_SER_R1 ((void*)&regs[0][2])
#define MIB_SER_R2 ((void*)&regs[0][3])
#define MIB_SER_R3 ((void*)&regs[0][4])
#define MIB_SER_CR_PPE1 ((void*)&regs[1][0])
#define MIB_SER_R0_PPE1 ((void*)&regs[1][1])
#define MIB_SER_R1_PPE1 ((void*)&regs[1][2])
#define MIB_SER_R2_PPE1 ((void*)&regs[1][3])
#define MIB_SER_R3_PPE1 ((void*)&regs[1][4])
static u32 reg_read(void *p) {
 unsigned n=p>=(void*)&regs[1][0];reads[n]++;
 if(p==(void*)&regs[n][0]) {
  if(mode[n]==1) return BIT(16);
  if(pending[n]) {if(mode[n]==2)return BIT(16);pending[n]--;if(!pending[n])regs[n][0]=0;}
 }
 return *(u32*)p;
}
static void reg_write(void*p,u32 value) {unsigned n=p>=(void*)&regs[1][0];writes[n]++;*(u32*)p=value;pending[n]=2;}
#define read_poll_timeout_atomic(op,val,cond,delay,timeout,sleep,addr) ({int result=-110;for(unsigned attempt=0;attempt<8;attempt++){val=op(addr);if(cond){result=0;break;}}result;})
#define wmb() ((void)0)
#define pr_warn_ratelimited(...) (++warnings)
#define HNAT_MIB_TIMEOUT_US 1000
struct ppe_mib_guard {
 spinlock_t lock;
 unsigned long retry_after;
 atomic64_t timeouts, contended, backoff;
};
static struct ppe_mib_guard ppe_mib_guards[2] = {
 { .lock = __SPIN_LOCK_UNLOCKED(ppe_mib_guards[0].lock) },
 { .lock = __SPIN_LOCK_UNLOCKED(ppe_mib_guards[1].lock) },
};

static void ppe_mib_read(unsigned int ppe, unsigned int entry_num,
                         u64 *pkt_cnt, u64 *byte_cnt)
{
 struct ppe_mib_guard *g = &ppe_mib_guards[ppe];
 unsigned long flags;
 u32 value, r0, r1, r2;
 void __iomem *cr = ppe ? MIB_SER_CR_PPE1 : MIB_SER_CR;
 void __iomem *r0reg = ppe ? MIB_SER_R0_PPE1 : MIB_SER_R0;
 void __iomem *r1reg = ppe ? MIB_SER_R1_PPE1 : MIB_SER_R1;
 void __iomem *r2reg = ppe ? MIB_SER_R2_PPE1 : MIB_SER_R2;
#if defined(CONFIG_HNAT_V2)
 u32 r3;
 void __iomem *r3reg = ppe ? MIB_SER_R3_PPE1 : MIB_SER_R3;
#endif
 bool failed = false;
 *pkt_cnt = 0;
 *byte_cnt = 0;
 if (!spin_trylock_irqsave(&g->lock, flags)) {
  atomic64_inc(&g->contended);
  return;
 }
 if (g->retry_after && time_before(jiffies, g->retry_after)) {
  atomic64_inc(&g->backoff);
  goto out;
 }
 /* Do not overwrite a previous, late hardware transaction while still busy. */
 if (read_poll_timeout_atomic(reg_read, value, !(value & BIT(16)),
                             0, HNAT_MIB_TIMEOUT_US, false, cr))
  goto timeout;
 reg_write(cr, entry_num | BIT(16));
 if (read_poll_timeout_atomic(reg_read, value, !(value & BIT(16)),
                             0, HNAT_MIB_TIMEOUT_US, false, cr))
  goto timeout;
 wmb();
 r0 = reg_read(r0reg);
 r1 = reg_read(r1reg);
 r2 = reg_read(r2reg);
#if defined(CONFIG_HNAT_V1)
 *pkt_cnt = ((u64)(r2 & 0xffffff) << 16) | (r1 >> 16);
 *byte_cnt = ((u64)(r1 & 0xffff) << 32) | r0;
#elif defined(CONFIG_HNAT_V2)
 r3 = reg_read(r3reg);
 *pkt_cnt = ((u64)r3 << 32) | r2;
 *byte_cnt = ((u64)r1 << 32) | r0;
#endif
 g->retry_after = 0;
 goto out;
timeout:
 atomic64_inc(&g->timeouts);
 g->retry_after = jiffies + HZ;
 failed = true;
out:
 spin_unlock_irqrestore(&g->lock, flags);
 if (failed)
  pr_warn_ratelimited("hwnat: PPE%u MIB statistics timeout; forwarding unchanged, retry after 1s\n", ppe);
}

void ppe_mib_dump_ppe0(unsigned int entry_num, u64 *pkt_cnt, u64 *byte_cnt)
{
 ppe_mib_read(0, entry_num, pkt_cnt, byte_cnt);
}

void ppe_mib_dump_ppe1(unsigned int entry_num, u64 *pkt_cnt, u64 *byte_cnt)
{
 ppe_mib_read(1, entry_num, pkt_cnt, byte_cnt);
}

void ppe_mib_print_health(struct seq_file *s)
{
 unsigned int i;
 for (i = 0; i < ARRAY_SIZE(ppe_mib_guards); i++) {
  struct ppe_mib_guard *g = &ppe_mib_guards[i];
  seq_printf(s, "ppe%u_mib_timeouts %lld\nppe%u_mib_contended %lld\nppe%u_mib_backoff %lld\n",
             i, atomic64_read(&g->timeouts), i, atomic64_read(&g->contended),
             i, atomic64_read(&g->backoff));
 }
}


static void reset(void) {
 memset(regs,0,sizeof regs);memset(mode,0,sizeof mode);memset(pending,0,sizeof pending);
 memset(reads,0,sizeof reads);memset(writes,0,sizeof writes);memset(ppe_mib_guards,0,sizeof ppe_mib_guards);warnings=0;jiffies=100;
 for(int p=0;p<2;p++){regs[p][1]=0x12345678;regs[p][2]=0x99abcdef;regs[p][3]=0x76543210;regs[p][4]=0x13579bdf;}
}
int main(void) {
 u64 packets,bytes;unsigned before;reset();
 for(int i=0;i<8000;i++) {
  ppe_mib_dump_ppe0(123,&packets,&bytes);
#if defined(CONFIG_HNAT_V2)
  assert(packets==UINT64_C(0x13579bdf76543210));assert(bytes==UINT64_C(0x99abcdef12345678));
#else
  assert(packets==UINT64_C(0x54321099ab));assert(bytes==UINT64_C(0xcdef12345678));
#endif
 }
 assert(!warnings&&!ppe_mib_guards[0].timeouts.value&&writes[0]==8000);
 mode[0]=1;before=writes[0];ppe_mib_dump_ppe0(1,&packets,&bytes);
 assert(!packets&&!bytes&&writes[0]==before&&warnings==1&&ppe_mib_guards[0].timeouts.value==1);
 before=reads[0];for(int i=0;i<1000;i++)ppe_mib_dump_ppe0(2,&packets,&bytes);
 assert(reads[0]==before&&ppe_mib_guards[0].backoff.value==1000);
 ppe_mib_dump_ppe1(3,&packets,&bytes);assert(packets&&bytes&&!ppe_mib_guards[1].timeouts.value);
 jiffies+=HZ;mode[0]=0;pending[0]=0;regs[0][0]=0;ppe_mib_dump_ppe0(4,&packets,&bytes);assert(packets&&bytes&&!ppe_mib_guards[0].retry_after);
 reset();mode[0]=2;ppe_mib_dump_ppe0(5,&packets,&bytes);assert(!packets&&!bytes&&writes[0]==1&&warnings==1);
 jiffies+=HZ;before=writes[0];ppe_mib_dump_ppe0(6,&packets,&bytes);assert(writes[0]==before&&warnings==2);
 mode[0]=0;jiffies+=HZ;ppe_mib_dump_ppe0(7,&packets,&bytes);assert(packets&&bytes);
 ppe_mib_guards[0].lock=1;before=writes[0];ppe_mib_dump_ppe0(8,&packets,&bytes);
 assert(!packets&&!bytes&&writes[0]==before&&ppe_mib_guards[0].contended.value==1&&ppe_mib_guards[0].lock==1);
 puts("actual-MIB-functions: normal 8000 reads; pre/post timeout; backoff; recovery; PPE isolation; contention passed");return 0;
}
