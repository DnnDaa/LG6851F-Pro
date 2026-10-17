#!/bin/sh
. /lib/netifd/netifd-wireless.sh
. /lib/netifd/hostapd.sh
init_wireless_driver "$@"
N='
'
drv_vendor618_init_device_config() {
 hostapd_common_add_device_config
 config_add_string phy
 config_add_int txpower beacon_int frag rts
 config_add_string distance
 config_add_boolean noscan rxldpc ldpc
}
drv_vendor618_init_iface_config() {
 hostapd_common_add_bss_config
 config_add_int dtim_period
 config_add_boolean wmm
}
drv_vendor618_init_vlan_config() { :; }
drv_vendor618_init_station_config() { :; }
drv_vendor618_cleanup() { :; }
vendor_identity() {
 case "$1" in
  radio0) vif=ra0; band_id=24; phy=phy0; expected_band=2g; hw=g ;;
  radio1) vif=rai0; band_id=5; phy=phy1; expected_band=5g; hw=a ;;
  *) return 1 ;;
 esac
 pidfile=/var/run/lg6851f-vendor618-hostapd${band_id}.pid
 conf=/tmp/lg6851f-vendor618/hostapd${band_id}.conf
}
vendor_stop() {
 local pid n=0
 pid=$(cat "$pidfile" 2>/dev/null)
 case "$pid" in ''|*[!0-9]*) return 0;; esac
 if [ "$(readlink -f /proc/$pid/exe | sed 's/ (deleted)$//')" = "$(readlink -f /usr/sbin/hostapd)" ] &&
    tr '\000' ' ' < /proc/$pid/cmdline | grep -Fq "$conf"; then
  kill "$pid" 2>/dev/null || true
  while kill -0 "$pid" 2>/dev/null && [ "$n" -lt 10 ]; do
   sleep 1; n=$((n+1))
  done
  kill -0 "$pid" 2>/dev/null && return 1
 fi
 rm -f "$pidfile"
 return 0
}
vendor_acl() {
 local policy=0 entry
 case "$macfilter" in allow) policy=1;; deny) policy=2;; esac
 /usr/sbin/lg6851f-mwctl "$vif" set ACLClearAll=1 || return 1
 for entry in $maclist; do
  /usr/sbin/lg6851f-mwctl "$vif" set "ACLAddEntry=$entry" || return 1
 done
 /usr/sbin/lg6851f-mwctl "$vif" set "AccessPolicy=$policy"
}
vendor_setup_vif() {
 local name="$1" ssid key mode network size pid hidden isolate wmm dtim_period macfilter maclist
 local hostapd_cfg='' fingerprint actual global_country n=0 ifname="$vif"
 count=$((count+1))
 json_select config
 json_get_vars ssid key mode hidden isolate wmm dtim_period macfilter
 json_get_values maclist maclist
 json_get_values network network
 if [ "$mode" != ap ] || [ "$network" != lan ]; then
  json_select ..; failed=AP_LAN_REQUIRED; return
 fi
 size=$(printf %s "$ssid" | wc -c)
 if [ "$size" -lt 1 ] || [ "$size" -gt 32 ]; then
  json_select ..; failed=INVALID_SSID; return
 fi
 case "$ssid$key" in *"$(printf '\r')"*|*'
'*) json_select ..; failed=INVALID_LINE_ENDING; return;; esac
 # OpenWrt translates security, MAC ACL, isolation, 802.11r/k/v, rekey,
 # inactivity, WPS and RADIUS options using its own hostapd helper.
 hostapd_set_bss_options hostapd_cfg "$phy" "$name" || {
  json_select ..; failed=HOSTAPD_CONFIGURATION; return;
 }
 # mac80211 supplies this identity outside the common hostapd helper.
 # FT needs a stable NAS ID; preserve an explicitly configured one.
 case "$N$hostapd_cfg" in *"${N}nas_identifier="*) ;;
  *) append hostapd_cfg "nas_identifier=$(cat /sys/class/net/$vif/address | tr -d :)" "$N";;
 esac
 json_select ..
 hidden=${hidden:-0}; isolate=${isolate:-0}; wmm=${wmm:-1}; dtim_period=${dtim_period:-2}
 umask 077
 mkdir -p /tmp/lg6851f-vendor618
 json_load "$data"
 json_select config
 hostapd_prepare_device_config "$conf.new" nl80211
 json_select ..
 # Power-only changes do not require hostapd or the AP to restart.
 fingerprint=$(printf %s "$data" | /usr/sbin/lg6851f-wifi-power fingerprint) || {
  failed=CONFIG_FINGERPRINT_FAILED; return;
 }
 fingerprint=$(printf %s "$fingerprint" | sha256sum | cut -d' ' -f1)
 {
  printf 'interface=%s\nbridge=br-lan\nchannel=%s\nieee80211n=1\n' "$vif" "$start_channel"
  printf '%s\n' "$hostapd_cfg"
  printf 'wmm_enabled=%s\ndtim_period=%s\n# vendor618-config=%s\n' "$wmm" "$dtim_period" "$fingerprint"
 } >> "$conf.new"
 pid=$(cat "$pidfile" 2>/dev/null)
 case "$pid" in ''|*[!0-9]*) pid=999999;; esac
 if ! cmp -s "$conf.new" "$conf" || [ "$(cat "$conf.applied" 2>/dev/null)" != "$fingerprint" ] || ! kill -0 "$pid" 2>/dev/null; then
  vendor_stop || { failed=HOSTAPD_STOP_TIMEOUT; return; }
  mv "$conf.new" "$conf"
  ln -sf "$conf" "/var/run/hostapd-$phy.conf"
  # stop_ap clears vendor PHY state; force ndo_open to restore it.
  ip link set "$vif" down || { failed=LINK_DOWN_FAILED; return; }
  ip link set "$vif" up || { failed=LINK_UP_FAILED; return; }
  global_country=$(iw reg get | sed -n '/^global$/,/^phy#/s/^country \([A-Z0-9][A-Z0-9]\):.*/\1/p' | head -n 1)
  if [ "$global_country" != "$country" ]; then
   iw reg set "$country" || { failed=REGULATORY_FAILED; return; }
  fi
  /usr/sbin/lg6851f-mwctl "$phy" set country "code=$country" || { failed=COUNTRY_FAILED; return; }
  ip link set "$vif" master br-lan || { failed=BRIDGE_FAILED; return; }
  vendor_acl || { failed=MAC_ACL_FAILED; return; }
  /usr/sbin/lg6851f-mwctl "$vif" set Debug=0 || true
  hostapd -B -P "$pidfile" "$conf" || { failed=HOSTAPD_FAILED; return; }
  /usr/sbin/lg6851f-mwctl "$vif" set phymode "$vendor_mode" || { failed=PHY_MODE_FAILED; return; }
  vendor_acl || { failed=MAC_ACL_FAILED; return; }
  /usr/sbin/lg6851f-mwctl "$phy" set channel "num=$start_channel" "bw=$width" "ext_chan=$ext_chan" || { failed=CHANNEL_FAILED; return; }
  /usr/sbin/lg6851f-mwctl "$vif" set "NoForwarding=$isolate" || { failed=ISOLATION_FAILED; return; }
  /usr/sbin/lg6851f-mwctl "$vif" set hide_ssid "$hidden" || { failed=HIDDEN_SSID_FAILED; return; }
  /usr/sbin/lg6851f-mwctl "$vif" set dtim_int "$dtim_period" || { failed=DTIM_FAILED; return; }
  /usr/sbin/lg6851f-mwctl "$vif" set "WmmCapable=$wmm" || { failed=WMM_FAILED; return; }
  /usr/sbin/lg6851f-mwctl "$vif" set "RTSThreshold=${rts:-2347}" || { failed=RTS_FAILED; return; }
  /usr/sbin/lg6851f-mwctl "$vif" set "FragThreshold=${frag:-2346}" || { failed=FRAG_FAILED; return; }
  /usr/sbin/lg6851f-mwctl "$vif" set "HtBssCoex=$((1-${noscan:-0}))" || { failed=COEX_FAILED; return; }
  /usr/sbin/lg6851f-mwctl "$vif" set "HtLdpc=${ldpc:-1}" || { failed=LDPC_FAILED; return; }
  # Like the AX6000 reference, leave MCS/NSS/GI selection to vendor RA.
  # 33 is the profile AUTO sentinel, not HE MCS 33. Re-arm on full AP setup
  # only: power-only saves must not reset rate adaptation. Keep BE/160 and
  # the MT7990 calibration/aggregation capabilities intact.
  if ! /usr/sbin/lg6851f-mwctl "$vif" set HtMcs=33 ||
     ! /usr/sbin/lg6851f-mwctl "$vif" set AutoRatePerBSS=1-15; then
   logger -t vendor618 "$__netifd_device: could not restore vendor automatic rate control"
  fi
  case "$distance" in ''|auto) ;; *) /usr/sbin/lg6851f-mwctl "$vif" set "Distance=$distance" || { failed=DISTANCE_FAILED; return; };; esac
  if [ "$auto_channel" = 1 ]; then
   /usr/sbin/lg6851f-mwctl "$vif" set AutoChannelSel=3 || { failed=ACS_FAILED; return; }
  fi
 else
  rm -f "$conf.new"
 fi
 while [ "$n" -lt 10 ]; do
  actual=$(iw dev "$vif" info | sed -n 's/.*channel \([0-9]*\) .*/\1/p')
  [ "$actual" = "$start_channel" ] && break
  [ "$auto_channel" != 1 ] || break
  sleep 1; n=$((n+1))
 done
 if [ "$auto_channel" != 1 ] && [ "$actual" != "$start_channel" ]; then
  failed=CHANNEL_NOT_APPLIED; return
 fi
 # Clear legacy software reduction before applying an absolute target.
 # This also runs when only txpower changed and the existing AP stays up.
 /usr/sbin/lg6851f-mwctl "$vif" set pwr TxPower=100 || { failed=TXPOWER_PERCENT_FAILED; return; }
 /usr/sbin/lg6851f-mwctl "$vif" set pwr DecreasePower=0 || { failed=TXPOWER_FAILED; return; }
 if ! /usr/sbin/lg6851f-wifi-power apply "$__netifd_device" "${txpower:-auto}" >/dev/null; then
  # The helper restores the prior compensation on any failed readback.
  # Keep the AP available; the wireless form reports the failure via RPC.
  logger -t vendor618 "$__netifd_device: power target not applied; inspect firmware power status"
 fi
 printf '%s\n' "$fingerprint" > "$conf.applied"
 wireless_add_vif "$name" "$vif"
 wireless_add_process "$(cat "$pidfile")" /usr/sbin/hostapd 1 1
 # Restore the current vif expected by for_each_interface.
 json_load "$data"
 json_select interfaces
 json_select "$name"
}
drv_vendor618_setup() {
 vendor_identity "$1" || { wireless_setup_failed UNKNOWN_RADIO; return; }
 [ -e /tmp/lg6851f-vendor618-ready ] && [ -e /sys/class/net/$vif ] || {
  wireless_setup_failed VENDOR_MODULES_NOT_READY; return;
 }
 local band country htmode width vendor_mode start_channel ext_chan txpower rts frag distance noscan rxldpc ldpc count=0 failed actual keys
 json_select config
 json_get_vars band channel country htmode txpower rts frag distance noscan rxldpc ldpc
 json_select ..
 country=$(printf %s "${country:-AU}" | tr a-z A-Z)
 # An unavailable LuCI capability response must not erase the working mode.
 if [ -z "$htmode" ]; then
  case "$band" in 2g) htmode=EHT40;; 5g) htmode=EHT160;; esac
 fi
 [ "$band" = "$expected_band" ] || { wireless_setup_failed INVALID_BAND; return; }
 case "$country" in [A-Z][A-Z]|00) ;; *) wireless_setup_failed INVALID_COUNTRY; return;; esac
 case "$band:$htmode" in
  2g:HT20|2g:HT40*|2g:HE20|2g:HE40*|2g:EHT20|2g:EHT40*) ;;
  5g:HT20|5g:HT40*|5g:VHT20|5g:VHT40|5g:VHT80|5g:VHT160|5g:HE20|5g:HE40|5g:HE80|5g:HE160|5g:EHT20|5g:EHT40|5g:EHT80|5g:EHT160) ;;
  *) wireless_setup_failed UNSUPPORTED_CHANNEL_WIDTH; return ;;
 esac
 width=$(printf %s "$htmode" | tr -cd '0-9')
 case "$band:$htmode" in
  2g:EHT*) vendor_mode=22;; 5g:EHT*) vendor_mode=23;;
  2g:HE*) vendor_mode=16;; 5g:HE*) vendor_mode=17;;
  2g:HT*) vendor_mode=9;; 5g:HT*) vendor_mode=8;; 5g:VHT*) vendor_mode=14;;
 esac
 auto_channel=0
 case "$channel" in auto|0|'') auto_channel=1;; esac
 start_channel=$channel
 if [ "$auto_channel" = 1 ]; then
  case "$band" in 2g) start_channel=6;; 5g) start_channel=36;; esac
 fi
 case "$start_channel" in ''|*[!0-9]*) wireless_setup_failed INVALID_CHANNEL; return;; esac
 [ "$start_channel" -ge 1 ] && [ "$start_channel" -le 177 ] || { wireless_setup_failed INVALID_CHANNEL; return; }
 ext_chan=above
 case "$htmode" in *-) ext_chan=below;; *) [ "$band" != 2g ] || [ "$start_channel" -le 7 ] || ext_chan=below;; esac
 case "$txpower" in ''|[0-9]|[12][0-9]|30) ;; *) wireless_setup_failed INVALID_TXPOWER; return;; esac
 json_get_keys keys interfaces
 set -- $keys
 [ "$#" = 1 ] || { wireless_setup_failed ONE_AP_PER_BAND_REQUIRED; return; }
 for_each_interface '' vendor_setup_vif
 if [ -n "$failed" ]; then
  logger -t vendor618 "$__netifd_device: $failed"
  wireless_setup_failed "$failed"
  return
 fi
 wireless_set_data "phy=$phy"
 # WED IRQs are allocated when the radio opens, after early boot steering.
 [ ! -x /usr/sbin/lg6851f-wifi-steering ] || /usr/sbin/lg6851f-wifi-steering || logger -t vendor618 'Could not apply wireless CPU steering'
 wireless_set_up
}
drv_vendor618_teardown() {
 vendor_identity "$1" || return
 vendor_stop || return
 [ ! -e /sys/class/net/$vif ] || ip link set "$vif" down
}
add_driver vendor618
