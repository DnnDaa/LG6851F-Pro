// SPDX-License-Identifier: GPL-2.0
/*
 * MT6990 adaptation of MediaTek's hardware CPUFreq interface.
 * Register protocol/LUT: MediaTek F370 SDK mediatek-cpufreq-hw.c (2020).
 * Physical clock decoding: MediaTek cpufreq_v1/cpufreq-dbg.c.
 * Cooling uses the kernel thermal core and a FREQ_QOS_MAX constraint.
 */
#include <linux/bitfield.h>
#include <linux/cpu.h>
#include <linux/cpufreq.h>
#include <linux/io.h>
#include <linux/iopoll.h>
#include <linux/math64.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/platform_device.h>
#include <linux/pm_qos.h>
#include <linux/slab.h>
#include <linux/thermal.h>

#define LUT_COUNT 32
#define FREQ_ENABLE 0x84
#define FREQ_REQUEST 0x88
#define FREQ_STATE 0x8c
#define FREQ_LATENCY 0x110

/* Preserve the last probe result even if Wi-Fi overwrites the boot log ring. */
static int probe_errno;
static char *probe_stage = "not_probed";
module_param(probe_errno, int, 0444);
module_param(probe_stage, charp, 0444);

struct lg_cpu {
    struct device *dev;
    void __iomem *base, *pll, *divider;
    u32 pll_offset, divider_offset, saved_enable, saved_index;
    struct cpufreq_frequency_table table[LUT_COUNT + 1];
    unsigned int count;
    cpumask_t cpus;
    struct thermal_cooling_device *cooling;
    struct freq_qos_request thermal_max;
    unsigned long cooling_state;
    bool qos_active, enabled;
};

/* Firmware supplies the voltages and programs the PLL; AP only requests an OPP. */
static unsigned int physical_khz(struct lg_cpu *d)
{
    u32 con1, div, postdiv;
    u64 f;
    if (!d->pll || !d->divider)
        return 0;
    con1 = readl(d->pll + d->pll_offset);
    div = FIELD_GET(GENMASK(21, 17), readl(d->divider + d->divider_offset));
    postdiv = FIELD_GET(GENMASK(26, 24), con1);
    if (postdiv > 4)
        return 0;
    f = div_u64((u64)(con1 & GENMASK(21, 0)) * 26000,
                16384U << postdiv);
    if (div >= 9 && div <= 11)
        f = div_u64(f * (12 - div), 4);
    else if (div >= 17 && div <= 20)
        f = div_u64(f * (21 - div), 5);
    else if (div >= 25 && div <= 29)
        f = div_u64(f * (30 - div), 6);
    else if (div != 0 && div != 8 && div != 16 && div != 24)
        return 0;
    return f;
}

static ssize_t pll_show(struct cpufreq_policy *policy, char *buf)
{
    return sysfs_emit(buf, "%u\n", physical_khz(policy->driver_data));
}
static ssize_t cooling_show(struct cpufreq_policy *policy, char *buf)
{
    struct lg_cpu *d = policy->driver_data;
    return sysfs_emit(buf, "%u\n", !!d->cooling);
}
static struct freq_attr pll_attr = {
    .attr = { .name = "cpu_pll_cur_freq", .mode = 0444 }, .show = pll_show,
};
static struct freq_attr cooling_attr = {
    .attr = { .name = "hardware_cooling_ready", .mode = 0444 }, .show = cooling_show,
};
static struct freq_attr *lg_attrs[] = {
    /* Linux 6.18 creates scaling_available_frequencies from policy->freq_table. */
    &pll_attr, &cooling_attr, NULL,
};

static int cooling_max(struct thermal_cooling_device *c, unsigned long *state)
{
    struct lg_cpu *d = c->devdata;
    *state = d->count - 1;
    return 0;
}
static int cooling_get(struct thermal_cooling_device *c, unsigned long *state)
{
    struct lg_cpu *d = c->devdata;
    *state = d->cooling_state;
    return 0;
}
static int cooling_set(struct thermal_cooling_device *c, unsigned long state)
{
    struct lg_cpu *d = c->devdata;
    int ret;
    if (state >= d->count)
        return -EINVAL;
    ret = freq_qos_update_request(&d->thermal_max, d->table[state].frequency);
    if (ret < 0)
        return ret;
    d->cooling_state = state;
    return 0;
}
static const struct thermal_cooling_device_ops lg_cooling_ops = {
    .get_max_state = cooling_max, .get_cur_state = cooling_get,
    .set_cur_state = cooling_set,
};

static void lg_ready(struct cpufreq_policy *policy)
{
    struct lg_cpu *d = policy->driver_data;
    struct device_node *np;
    int ret;
    ret = freq_qos_add_request(&policy->constraints, &d->thermal_max,
                              FREQ_QOS_MAX, d->table[0].frequency);
    if (ret < 0) {
        dev_err(d->dev, "thermal frequency constraint failed: %d\n", ret);
        return;
    }
    d->qos_active = true;
    np = of_get_cpu_node(policy->cpu, NULL);
    if (np && of_property_present(np, "#cooling-cells"))
        d->cooling = thermal_of_cooling_device_register(np, "lg6851f-cpufreq",
                                                       d, &lg_cooling_ops);
    of_node_put(np);
    if (IS_ERR_OR_NULL(d->cooling)) {
        d->cooling = NULL;
        freq_qos_update_request(&d->thermal_max, d->table[d->count - 1].frequency);
        dev_err(d->dev, "no DT CPU cooling device; capped at lowest OPP\n");
    }
}

static int lg_target(struct cpufreq_policy *policy, unsigned int index)
{
    struct lg_cpu *d = policy->driver_data;
    if (index >= d->count)
        return -EINVAL;
    writel(index, d->base + FREQ_REQUEST);
    return 0;
}
static unsigned int lg_fast(struct cpufreq_policy *policy, unsigned int khz)
{
    struct lg_cpu *d = policy->driver_data;
    unsigned int index = cpufreq_table_find_index_dl(policy, khz, false);
    if (index >= d->count)
        return 0;
    writel(index, d->base + FREQ_REQUEST);
    return d->table[index].frequency;
}
static unsigned int lg_get(unsigned int cpu)
{
    struct lg_cpu *d = cpufreq_get_driver_data();
    unsigned int index;
    if (!d || !cpumask_test_cpu(cpu, &d->cpus))
        return 0;
    index = readl(d->base + FREQ_REQUEST);
    return index < d->count ? d->table[index].frequency : 0;
}
static void restore_firmware(struct lg_cpu *d)
{
    if (!d->enabled)
        return;
    writel(d->saved_index, d->base + FREQ_REQUEST);
    writel(d->saved_enable, d->base + FREQ_ENABLE);
    d->enabled = false;
}
static int lg_init(struct cpufreq_policy *policy)
{
    struct lg_cpu *d = cpufreq_get_driver_data();
    u32 sig, latency;
    int ret;
    probe_stage = "policy_init";
    cpumask_copy(policy->cpus, &d->cpus);
    policy->driver_data = d;
    policy->freq_table = d->table;
    latency = readl(d->base + FREQ_LATENCY);
    policy->cpuinfo.transition_latency = latency && latency < 1000000 ?
        latency * 1000 : CPUFREQ_DEFAULT_TRANSITION_LATENCY_NS;
    policy->fast_switch_possible = true;
    d->saved_enable = readl(d->base + FREQ_ENABLE);
    d->saved_index = readl(d->base + FREQ_REQUEST);
    if (d->saved_index >= d->count)
        return dev_err_probe(d->dev, -EINVAL, "invalid initial frequency index %u\n",
                             d->saved_index);
    writel(1, d->base + FREQ_ENABLE);
    d->enabled = true;
    ret = readl_poll_timeout(d->base + FREQ_STATE, sig, sig & BIT(0), 1000, 300000);
    if (ret) {
        restore_firmware(d);
        return dev_err_probe(d->dev, -ENODEV, "MCUPM CPU DVFS not ready\n");
    }
    /* Match the vendor driver: SVS bit is optional, CPU hardware bit is required. */
    if (!(sig & BIT(1)))
        dev_info(d->dev, "SVS unavailable; retaining firmware voltage policy\n");
    probe_stage = "policy_hardware_ready";
    return 0;
}
static void lg_exit(struct cpufreq_policy *policy)
{
    struct lg_cpu *d = policy->driver_data;
    if (d->cooling) {
        thermal_cooling_device_unregister(d->cooling);
        d->cooling = NULL;
    }
    if (d->qos_active) {
        freq_qos_remove_request(&d->thermal_max);
        d->qos_active = false;
    }
    restore_firmware(d);
}
static struct cpufreq_driver lg_driver = {
    .name = "lg6851f-mtk-hw",
    .flags = CPUFREQ_NEED_INITIAL_FREQ_CHECK | CPUFREQ_HAVE_GOVERNOR_PER_POLICY,
    .verify = cpufreq_generic_frequency_table_verify,
    .target_index = lg_target, .fast_switch = lg_fast, .get = lg_get,
    .init = lg_init, .exit = lg_exit, .ready = lg_ready, .attr = lg_attrs,
};

static void map_physical_clock(struct lg_cpu *d)
{
    struct device_node *hv, *np;
    u32 ver;
    hv = of_find_compatible_node(NULL, NULL, "mediatek,cpufreq-hybrid");
    if (!hv)
        return;
    if (of_property_read_u32(hv, "mcucfg-ver", &ver) || ver != 0 ||
        of_property_read_u32(hv, "pll-con", &d->pll_offset) ||
        of_property_read_u32(hv, "clk-div", &d->divider_offset))
        goto out;
    if (d->pll_offset != 0x2f4 || d->divider_offset != 0xa2a0)
        goto out;
    np = of_parse_phandle(hv, "apmixedsys", 0);
    d->pll = of_iomap(np, 0);
    of_node_put(np);
    np = of_parse_phandle(hv, "clk-div-base", 0);
    d->divider = of_iomap(np, 0);
    of_node_put(np);
out:
    of_node_put(hv);
}

static int lg_probe_inner(struct platform_device *pdev)
{
    struct lg_cpu *d;
    struct resource *res;
    struct device_node *np;
    struct of_phandle_args args;
    unsigned int cpu, i;
    u32 raw, freq, previous = 0;
    int ret;
    probe_stage = "resource_check";
    if (!of_machine_is_compatible("mediatek,mt6990"))
        return -ENODEV;
    res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
    if (!res || res->start != 0x11bc10 || resource_size(res) != 0x120)
        return -EINVAL;
    d = devm_kzalloc(&pdev->dev, sizeof(*d), GFP_KERNEL);
    if (!d)
        return -ENOMEM;
    d->dev = &pdev->dev;
    d->base = devm_platform_ioremap_resource(pdev, 0);
    if (IS_ERR(d->base))
        return PTR_ERR(d->base);
    probe_stage = "frequency_table";
    for (i = 0; i < LUT_COUNT; i++) {
        raw = readl(d->base + 4 * i);
        freq = (raw & 0xfff) * 1000;
        if (i && freq == previous)
            break;
        /* Reject missing/ascending/corrupt tables; never synthesize an OPP. */
        if (!freq || raw == 0xdeadba5e || freq > 2200000 ||
            freq < 100000 || (i && freq > previous))
            return dev_err_probe(d->dev, -EINVAL, "invalid firmware LUT row %u\n", i);
        d->table[i].frequency = freq;
        previous = freq;
    }
    if (i < 2 || i == LUT_COUNT)
        return dev_err_probe(d->dev, -EINVAL, "missing firmware LUT terminator\n");
    d->count = i;
    d->table[i].frequency = CPUFREQ_TABLE_END;
    probe_stage = "firmware_ready";
    if (!(readl(d->base + FREQ_STATE) & BIT(0)))
        return dev_err_probe(d->dev, -EPROBE_DEFER, "waiting for firmware DVFS\n");
    probe_stage = "shared_cpu_domain";
    for_each_possible_cpu(cpu) {
        np = of_cpu_device_node_get(cpu);
        if (!np)
            return -ENODEV;
        ret = of_parse_phandle_with_args(np, "performance-domains",
                                        "#performance-domain-cells", 0, &args);
        of_node_put(np);
        if (ret)
            return ret;
        if (args.np != pdev->dev.of_node || args.args_count != 1 || args.args[0]) {
            of_node_put(args.np);
            return -EINVAL;
        }
        of_node_put(args.np);
        cpumask_set_cpu(cpu, &d->cpus);
    }
    map_physical_clock(d);
    platform_set_drvdata(pdev, d);
    lg_driver.driver_data = d;
    probe_stage = "register_cpufreq";
    ret = cpufreq_register_driver(&lg_driver);
    if (ret) {
        restore_firmware(d);
        if (d->pll) iounmap(d->pll);
        if (d->divider) iounmap(d->divider);
        return ret;
    }
    dev_info(d->dev, "%u firmware OPPs, %u-%u kHz, shared CPUs %*pbl; PLL %u kHz\n",
             d->count, d->table[d->count - 1].frequency, d->table[0].frequency,
             cpumask_pr_args(&d->cpus), physical_khz(d));
    return 0;
}
static int lg_probe(struct platform_device *pdev)
{
    int ret = lg_probe_inner(pdev);
    probe_errno = ret;
    if (ret)
        return dev_err_probe(&pdev->dev, ret, "CPUFreq probe failed at %s\n", probe_stage);
    probe_stage = "ready";
    return 0;
}
static void lg_remove(struct platform_device *pdev)
{
    struct lg_cpu *d = platform_get_drvdata(pdev);
    cpufreq_unregister_driver(&lg_driver);
    if (d->pll) iounmap(d->pll);
    if (d->divider) iounmap(d->divider);
}
static const struct of_device_id lg_match[] = {
    { .compatible = "mediatek,cpufreq-hw" }, {},
};
MODULE_DEVICE_TABLE(of, lg_match);
static struct platform_driver lg_platform_driver = {
    .probe = lg_probe, .remove = lg_remove,
    .driver = { .name = "lg6851f-cpufreq-hw", .of_match_table = lg_match },
};
module_platform_driver(lg_platform_driver);
MODULE_DESCRIPTION("MT6990 firmware LUT CPUFreq and DT thermal cooling");
MODULE_LICENSE("GPL");
