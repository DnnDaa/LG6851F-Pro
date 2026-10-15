// SPDX-License-Identifier: GPL-2.0-only
/* Stock LG6851F hwcfg 10007050: OUT_POWER_LED=356, gpiochip offset=80.
 * GPIO16 is already owned by the stock DT class 5g_evb_voice (network LED).
 * Register only the missing power LED; never request the fan or supply GPIOs.
 */
#include <linux/module.h>
#include <linux/of.h>
#include <linux/platform_device.h>
#include <linux/gpio/machine.h>
#include <linux/leds.h>

static struct gpiod_lookup_table panel_lookup = {
	.dev_id = "leds-gpio",
	.table = {
		GPIO_LOOKUP("pinctrl_paris", 80, NULL, GPIO_ACTIVE_HIGH),
		{ },
	},
};

static const struct gpio_led panel_led = {
	.name = "lg6851f:power",
	.gpio = -1,
	.default_trigger = "default-on",
	.default_state = LEDS_GPIO_DEFSTATE_ON,
};

static const struct gpio_led_platform_data panel_data = {
	.num_leds = 1,
	.leds = &panel_led,
};

static struct platform_device *panel_device;

static int __init lg6851f_panel_init(void)
{
	if (!of_machine_is_compatible("fiberhome,lg6851f"))
		return -ENODEV;
	gpiod_add_lookup_table(&panel_lookup);
	panel_device = platform_device_register_data(NULL, "leds-gpio",
		PLATFORM_DEVID_NONE, &panel_data, sizeof(panel_data));
	if (IS_ERR(panel_device)) {
		gpiod_remove_lookup_table(&panel_lookup);
		return PTR_ERR(panel_device);
	}
	pr_info("lg6851f-panel-leds: registered stock power LED on GPIO80\n");
	return 0;
}

static void __exit lg6851f_panel_exit(void)
{
	platform_device_unregister(panel_device);
	gpiod_remove_lookup_table(&panel_lookup);
}

module_init(lg6851f_panel_init);
module_exit(lg6851f_panel_exit);
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("LG6851F stock front panel power LED");
MODULE_SOFTDEP("pre: leds-gpio");
