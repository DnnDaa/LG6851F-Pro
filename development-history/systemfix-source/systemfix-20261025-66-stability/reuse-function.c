/* Reconfigure a dormant single-BSS object without freeing global/PHY identity.
 * DISABLE has already released stations, WPA/SAE, MLD links and driver data.
 * Keep the control socket, interface and callback owners at stable addresses.
 * This deliberately does not call REMOVE, interface_free or driver teardown.
 */
int hostapd_lg6851f_reuse(struct hostapd_iface *iface,
                          const char *config_fname)
{
 struct hostapd_data *hapd;
 struct hostapd_config *next, *old;
 char *config_id = NULL;

 if (!iface || iface->state != HAPD_IFACE_DISABLED ||
     iface->num_bss != 1 || !iface->bss || !iface->bss[0] ||
     !config_fname || !config_fname[0] || !iface->interfaces ||
     !iface->interfaces->config_read_cb)
  return -1;
 hapd = iface->bss[0];
 if (hapd->drv_priv || hapd->started || hapd->num_sta ||
     hapd->wpa_auth || hapd->sta_list)
  return -1;
 old = iface->conf;
 next = iface->interfaces->config_read_cb(config_fname);
 if (!next)
  return -1;
 /* Control socket identity cannot change while keeping that socket alive. */
 if (next->num_bss != 1 || next->driver != old->driver ||
     os_strcmp(next->bss[0]->iface, old->bss[0]->iface) ||
     !next->bss[0]->ctrl_interface || !old->bss[0]->ctrl_interface ||
     os_strcmp(next->bss[0]->ctrl_interface, old->bss[0]->ctrl_interface) ||
     next->bss[0]->ctrl_interface_gid != old->bss[0]->ctrl_interface_gid ||
     next->bss[0]->ctrl_interface_gid_set != old->bss[0]->ctrl_interface_gid_set)
  goto reject;
 hostapd_set_security_params(next->bss[0], 1);
 if (hostapd_config_check(next, 1) < 0)
  goto reject;
 if (next->bss[0]->config_id) {
  config_id = os_strdup(next->bss[0]->config_id);
  if (!config_id)
   goto reject;
 }
 /* Same ownership transfer as reload_config, after complete BSS shutdown.
  * All allocations/checks precede publication, so failure keeps the old config.
  */
 iface->conf = next;
 hapd->iconf = next;
 hapd->conf = next->bss[0];
 hapd->driver = next->driver;
 os_free(hapd->config_id);
 hapd->config_id = config_id;
 hostapd_config_free(old);
 return 0;
reject:
 hostapd_config_free(next);
 return -1;
}
