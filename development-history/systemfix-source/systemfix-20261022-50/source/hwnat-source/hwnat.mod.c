#include <linux/module.h>
#include <linux/export-internal.h>
#include <linux/compiler.h>

MODULE_INFO(name, KBUILD_MODNAME);

__visible struct module __this_module
__section(".gnu.linkonce.this_module") = {
	.name = KBUILD_MODNAME,
	.init = init_module,
#ifdef CONFIG_MODULE_UNLOAD
	.exit = cleanup_module,
#endif
	.arch = MODULE_ARCH_INIT,
};

KSYMTAB_DATA(hnat_chip_name, "", "");
KSYMTAB_DATA(fe_feature, "", "");
KSYMTAB_FUNC(get_ppe_mib, "", "");
KSYMTAB_FUNC(get_ppe_mib_ip, "", "");
KSYMTAB_DATA(dbg_cpu_reason, "", "");
KSYMTAB_DATA(hnat_if, "", "");
KSYMTAB_DATA(dbg_cpu_reason_cnt, "", "");
KSYMTAB_DATA(hwnat_dbg_entry, "", "");
KSYMTAB_DATA(USE_3T_UDP_FRAG, "", "");
KSYMTAB_DATA(ppe_foe_base, "", "");
KSYMTAB_DATA(ppe1_foe_base, "", "");
KSYMTAB_FUNC(get_mib_entry_idx, "", "");
KSYMTAB_FUNC(_foe_drop_entry, "", "");
KSYMTAB_FUNC(foe_tbl_clean, "", "");
KSYMTAB_FUNC(foe_del_entry, "", "");

MODULE_INFO(depends, "");

