/* Test-only code, inserted into a separate runner, never into the firmware. */
static const char *test_reuse_path;
static int test_reuse_fault;
static struct hostapd_config *test_reuse_read(const char *unused)
{
 struct hostapd_config *c;
 (void) unused;
 if (test_reuse_fault == 1)
  return NULL;
 c = hostapd_config_read(test_reuse_path);
 if (c && test_reuse_fault == 2)
  os_strlcpy(c->bss[0]->iface, "wrong0", sizeof(c->bss[0]->iface));
 return c;
}
static int test_reuse_cycle(const char *first, const char *second)
{
 struct hapd_interfaces interfaces;
 struct hostapd_iface *iface;
 struct hostapd_data *hapd;
 struct hostapd_config *old;
 int i;
 os_memset(&interfaces, 0, sizeof(interfaces));
 interfaces.config_read_cb = hostapd_config_read;
 iface = hostapd_init(&interfaces, first);
 if (!iface) return 1;
 iface->interfaces = &interfaces;
 hapd = iface->bss[0];
 iface->state = HAPD_IFACE_DISABLED;
 interfaces.config_read_cb = test_reuse_read;
 test_reuse_path = second;
 old = iface->conf;
 test_reuse_fault = 1;
 if (!hostapd_lg6851f_reuse(iface, test_reuse_path) || iface->conf != old)
  return 2;
 test_reuse_fault = 2;
 if (!hostapd_lg6851f_reuse(iface, test_reuse_path) || iface->conf != old)
  return 3;
 test_reuse_fault = 0;
 hapd->started = 1;
 if (!hostapd_lg6851f_reuse(iface, test_reuse_path) || iface->conf != old)
  return 4;
 hapd->started = 0;
 hapd->drv_priv = hapd;
 if (!hostapd_lg6851f_reuse(iface, test_reuse_path) || iface->conf != old)
  return 5;
 hapd->drv_priv = NULL;
 iface->state = HAPD_IFACE_ENABLED;
 if (!hostapd_lg6851f_reuse(iface, test_reuse_path) || iface->conf != old)
  return 6;
 iface->state = HAPD_IFACE_DISABLED;
 for (i = 0; i < 4000; i++) {
  test_reuse_path = i & 1 ? first : second;
  if (hostapd_lg6851f_reuse(iface, test_reuse_path)) return 7;
  if (iface->bss[0] != hapd || hapd->iface != iface ||
      iface->conf != hapd->iconf || iface->conf->bss[0] != hapd->conf ||
      hapd->drv_priv || hapd->started) return 8;
 }
 printf("{\"passed\":true,\"cycles\":4000,\"reject_cases\":5,\"real_config_parser\":true,\"stable_object_pointers\":true}\n");
 return 0;
}
