// SPDX-License-Identifier: GPL-2.0-only
/* Read-only FOE table occupancy for the existing MT6990 vendor hwnat module.
 * No packet hooks, timers, register writes, table updates or printk polling.
 */
#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/string.h>
#include "foe_fdb.h"

extern struct foe_entry *ppe_foe_base;
extern struct foe_entry *ppe1_foe_base;
static struct proc_dir_entry *stats;

static int lg6851f_ppe_show(struct seq_file *seq, void *unused)
{
	struct foe_entry *tables[] = { READ_ONCE(ppe_foe_base), READ_ONCE(ppe1_foe_base) };
	unsigned int ppe, index;

	BUILD_BUG_ON(sizeof(struct bf_info_blk1) != sizeof(u32));
	for (ppe = 0; ppe < ARRAY_SIZE(tables); ppe++) {
		unsigned int states[4] = { 0 };
		if (!tables[ppe])
			continue;
		for (index = 0; index < FOE_4TB_SIZ; index++) {
			struct bf_info_blk1 snapshot;
			u32 word = READ_ONCE(*(u32 *)&tables[ppe][index]);
			/* Decode the vendor's exact compiled bitfield layout from one read. */
			memcpy(&snapshot, &word, sizeof(snapshot));
			states[snapshot.state]++;
		}
		seq_printf(seq, "PPE%u total=%u bound=%u unbound=%u fin=%u invalid=%u\n",
			   ppe, FOE_4TB_SIZ, states[2], states[1], states[3], states[0]);
	}
	return 0;
}
DEFINE_PROC_SHOW_ATTRIBUTE(lg6851f_ppe);

static int __init lg6851f_ppe_init(void)
{
	stats = proc_create("lg6851f_ppe_stats", 0400, NULL, &lg6851f_ppe_proc_ops);
	return stats ? 0 : -ENOMEM;
}

static void __exit lg6851f_ppe_exit(void)
{
	proc_remove(stats);
}

module_init(lg6851f_ppe_init);
module_exit(lg6851f_ppe_exit);
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Read-only LG6851F vendor PPE flow table occupancy");
MODULE_SOFTDEP("pre: hwnat");
