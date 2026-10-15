// SPDX-License-Identifier: GPL-2.0-only
/* MT6990 vendor PPE adapter for the open-source mt76 Wi-Fi driver. */
#include <linux/debugfs.h>
#include <linux/seq_file.h>
#include <linux/soc/mediatek/mtk_hnat.h>
#include "hnat_define.h"
#include "foe_fdb.h"
#include "hnat_common.h"

extern struct foe_entry *ppe_foe_base, *ppe1_foe_base;

int32_t ppe_tx_wifi_handler(struct sk_buff *skb, int gmac_no);
static atomic64_t seen = ATOMIC64_INIT(0), no_metadata = ATOMIC64_INIT(0);
static atomic64_t learned = ATOMIC64_INIT(0), attempted = ATOMIC64_INIT(0);
static atomic64_t consumed = ATOMIC64_INIT(0);
static struct dentry *mt76_debugfs;
static atomic64_t reasons[32], sources[16];

static bool mt6990_mt76_tx(struct sk_buff *skb, u8 wdma, u8 bss, u16 wcid, u8 ring)
{
 struct foe_entry *entry;
 bool was_bound;
 int ret;

 atomic64_inc(&seen);
 /* The caller has selected a unicast 802.3 hardware-encapsulation frame.
  * Do not manufacture FOE tags for CPU-originated or shared packets.
  */
 if (wdma > 1 || bss > 63 || wcid > 1023 || ring > 1 ||
     skb_cloned(skb) || skb_headroom(skb) < FOE_INFO_LEN ||
     !IS_MAGIC_TAG_PROTECT_VALID_HEAD(skb) ||
     (FOE_MAGIC_TAG(skb) < FOE_MAGIC_PCI || FOE_MAGIC_TAG(skb) > FOE_MAGIC_SNPS)) {
  atomic64_inc(&no_metadata);
  return false;
 }
 atomic64_inc(&reasons[FOE_AI(skb) & 31]);
 atomic64_inc(&sources[FOE_SP(skb) & 15]);
 if (FOE_AI(skb) != HIT_UNBIND_RATE_REACH &&
     FOE_AI(skb) != HIT_BIND_KEEPALIVE_DUP_OLD_HDR)
  return false;
 if (check_entry_region(skb))
  return false;
 entry = decide_which_ppe(skb);
 if (!entry)
  return false;
 was_bound = ppe_entry_check_state(entry, BIND);
 FOE_WDMA_ID(skb) = wdma;
 FOE_RX_ID(skb) = ring;
 FOE_WC_ID(skb) = wcid;
 FOE_BSS_ID(skb) = bss;
 if (FOE_AI(skb) == HIT_UNBIND_RATE_REACH)
  atomic64_inc(&attempted);
 ret = ppe_tx_wifi_handler(skb, wdma ? WDMA1_PSE_PORT : WDMA0_PSE_PORT);
 if (!was_bound && ppe_entry_check_state(entry, BIND))
  atomic64_inc(&learned);
 if (ret != 1)
  atomic64_inc(&consumed);
 return ret != 1;
}

static void mt6990_mt76_flush(const u8 *mac)
{
 foe_del_entry_by_mac((unsigned char *)mac);
}
static const struct mtk_hnat_wifi_ops mt76_ops = {
 .tx = mt6990_mt76_tx,
 .flush = mt6990_mt76_flush,
};
static int mt76_hnat_show(struct seq_file *s, void *unused)
{
 unsigned int i, ppe, bound[2] = {};
 struct foe_entry *tables[] = { ppe_foe_base, ppe1_foe_base };
 for (ppe = 0; ppe < ARRAY_SIZE(tables); ppe++) {
  if (!tables[ppe])
   continue;
  for (i = 0; i < FOE_4TB_SIZ; i++) {
   struct foe_entry *e = &tables[ppe][i];
   if (ppe_entry_check_state(e, BIND) &&
       (IS_IPV4_GRP(e) || IS_IPV6_GRP(e))) {
    unsigned int port = IS_IPV4_GRP(e) ? e->ipv4_hnapt.iblk2.dp :
                                                   e->ipv6_3t_route.iblk2.dp;
    if (port == WDMA0_PSE_PORT || port == WDMA1_PSE_PORT)
     bound[port == WDMA1_PSE_PORT]++;
   }
  }
 }
 seq_printf(s, "mode cellular-to-wifi-tx\nseen %lld\nno_metadata %lld\n"
     "bind_attempts %lld\nlearned %lld\nconsumed %lld\n",
     atomic64_read(&seen), atomic64_read(&no_metadata),
     atomic64_read(&attempted), atomic64_read(&learned),
     atomic64_read(&consumed));
 seq_printf(s, "bound_wdma0 %u\nbound_wdma1 %u\n", bound[0], bound[1]);
 for (i = 0; i < ARRAY_SIZE(reasons); i++)
  if (atomic64_read(&reasons[i]))
   seq_printf(s, "reason_%02x %lld\n", i, atomic64_read(&reasons[i]));
 for (i = 0; i < ARRAY_SIZE(sources); i++)
  if (atomic64_read(&sources[i]))
   seq_printf(s, "source_%u %lld\n", i, atomic64_read(&sources[i]));
 return 0;
}
DEFINE_SHOW_ATTRIBUTE(mt76_hnat);
void mt76_hnat_init(void)
{
 int ret = mtk_hnat_wifi_register(&mt76_ops);
 if (ret) {
  pr_err("mt6990-hwnat: mt76 adapter registration failed: %d\n", ret);
  return;
 }
 mt76_debugfs = debugfs_create_dir("mt6990_hnat", NULL);
 debugfs_create_file("wifi_offload", 0400, mt76_debugfs, NULL, &mt76_hnat_fops);
}
void mt76_hnat_exit(void)
{
 mtk_hnat_wifi_unregister(&mt76_ops);
 debugfs_remove_recursive(mt76_debugfs);
}
