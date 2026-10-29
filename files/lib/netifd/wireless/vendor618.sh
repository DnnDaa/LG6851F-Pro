#!/bin/sh
. /lib/netifd/netifd-wireless.sh
. /lib/netifd/hostapd.sh
. /lib/netifd/vendor618-client64.sh
init_wireless_driver "$@"
N='
'
drv_vendor618_init_device_config() {
 hostapd_common_add_device_config
 config_add_string phy
 config_add_int txpower beacon_int frag rts
 config_add_string distance vendor_power_level
 config_add_boolean noscan rxldpc ldpc vendor_power_boost
}
drv_vendor618_init_iface_config() {
 hostapd_common_add_bss_config
 config_add_int dtim_period
 config_add_boolean wmm vendor_mlo
 config_add_string vendor_protocol
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
vendor_bss_identity() {
 local slot="$1"
 case "$slot" in [0-7]) ;; *) return 1;; esac
 case "$__netifd_device" in radio0) vif="ra$slot";; radio1) vif="rai$slot";; *) return 1;; esac
 if [ "$slot" = 0 ]; then
  pidfile=/var/run/lg6851f-vendor618-hostapd$band_id.pid
  conf=/tmp/lg6851f-vendor618/hostapd$band_id.conf
 else
  pidfile=/var/run/lg6851f-vendor618-hostapd$band_id-$slot.pid
  conf=/tmp/lg6851f-vendor618/hostapd$band_id-$slot.conf
 fi
}
vendor_ensure_bss() {
 if [ ! -e "/sys/class/net/$vif" ]; then
  iw phy "$phy" interface add "$vif" type __ap || return 1
 fi
 [ "$(basename "$(readlink -f /sys/class/net/$vif/phy80211)")" = "$phy" ]
}

vendor_generation() {
 local phase="$1"
 mkdir -p /tmp/lg6851f-vendor618 || return 1
 # A successful retry can retain all APs and never enter changing in this
 # invocation. Always close a previous incomplete generation on final success.
 if [ -z "$vendor_generation_id" ]; then
  if [ "$phase" = ready ] && grep -q '^ready ' "/tmp/lg6851f-vendor618/$__netifd_device.generation" 2>/dev/null; then return 0; fi
  vendor_generation_id="$(cut -d' ' -f1 /proc/uptime):$$"
 fi
 [ -n "$vendor_generation_id" ] || return 0
 printf '%s %s\n' "$phase" "$vendor_generation_id" > "/tmp/lg6851f-vendor618/$__netifd_device.generation.new" || return 1
 mv -f "/tmp/lg6851f-vendor618/$__netifd_device.generation.new" "/tmp/lg6851f-vendor618/$__netifd_device.generation" || return 1
 # Notify the existing shared sampler; never query stations in this handler.
 if [ "$phase" = ready ]; then printf '1' > /tmp/lg6851f-wireless.wake || true; fi
 return 0
}
vendor_link_is_up() {
 local flags
 flags=$(cat "/sys/class/net/$vif/flags" 2>/dev/null) || return 1
 case "$flags" in 0x[0-9a-fA-F]*) ;; *) return 1;; esac
 [ $((flags & 1)) = 1 ]
}
vendor_quiesce_band() {
 local slot=7
 vendor64_client_stop || return 1
 vendor_generation changing || return 1
 # Complete ALL removals before any new beacon/MLD is installed.
 while [ "$slot" -ge 0 ]; do
  vendor_bss_identity "$slot" || return 1
  vendor_stop || return 1
  if vendor_link_is_up; then ip link set "$vif" down || return 1; fi
  slot=$((slot-1))
 done
 vendor_bss_identity 0
}
vendor_layout_has_slot() {
 printf '%s\n' "$layout" | grep -q "^$1 "
}
vendor_stop_unwanted() {
 local slot=7 was_up
 while [ "$slot" -ge 0 ]; do
  vendor_bss_identity "$slot" || return 1
  if ! vendor_layout_has_slot "$slot"; then
   was_up=0
   vendor_link_is_up && was_up=1
   vendor_disable_unwanted || return 1
   if [ "$was_up" = 1 ] && vendor_link_is_up; then
    ip link set "$vif" down || return 1
   fi
   rm -f "$conf.applied" "$conf.protocol" "$conf.mld_applied"
  fi
  slot=$((slot-1))
 done
}
vendor_prepare_reconfigure() {
 local slot section mapped group runtime protocol total=0 seen_slots=
 # Stable slots may contain holes after deleting a middle AP. Validate every
 # mapping and reject duplicate slots without touching the live band.
 while IFS=" " read -r slot section mapped group runtime protocol; do
  [ -n "$section" ] || continue
  case "$slot" in [0-7]) ;; *) return 1;; esac
  case " $seen_slots " in *" $slot "*) return 1;; esac
  seen_slots="$seen_slots $slot"
  vendor_bss_identity "$slot" || return 1
  [ "$vif" = "$mapped" ] || return 1
  total=$((total+1))
 done <<EOF
$layout
EOF
 [ "$total" = 0 ] || vendor_auth_ready || return 1
 # Retire only BSS interfaces absent from this radio's desired layout.
 vendor_stop_unwanted || return 1
 vendor_bss_identity 0
}
vendor_verify_band() {
 local slot section mapped group runtime
 while IFS=" " read -r slot section mapped group runtime protocol; do
  [ -n "$section" ] || continue
  vendor_bss_identity "$slot" || return 1
  vendor_link_is_up && vendor_bss_running || return 1
 done <<EOF
$layout
EOF
}
vendor_band_failed() {
 logger -t vendor618 "$__netifd_device: $1; partial failure isolated, existing BSS retained"
 return 1
}
vendor_control_unlock() {
 # Release the open-file-description lock explicitly, then close our fd.
 # This also prevents an accidentally inherited fd from pinning the lock.
 flock -u 9 2>/dev/null || true
 exec 9>&-
}
vendor_recover_stale_mlo_lock() {
 local p f target pid cmd stopped=0
 target=$(readlink -f /var/lock/lg6851f-radio-control.lock 2>/dev/null) || return 0
 [ -n "$target" ] || return 0
 for p in /proc/[0-9]*; do
  pid=${p##*/}
  for f in "$p"/fd/*; do
   [ "$(readlink -f "$f" 2>/dev/null)" = "$target" ] || continue
   cmd=$(tr '\000' ' ' < "$p/cmdline" 2>/dev/null) || continue
   case "$cmd" in
    *"/usr/sbin/lg6851f-mwctl "*" set ap_mld "*) ;;
    *) continue ;;
   esac
   logger -t vendor618 "recovering stale ap_mld lock holder pid=$pid"
   kill "$pid" 2>/dev/null || true
   sleep 1
   kill -0 "$pid" 2>/dev/null && kill -9 "$pid" 2>/dev/null || true
   stopped=1
  done
 done
 [ "$stopped" = 0 ] || sleep 1
}
vendor_control_lock() {
 mkdir -p /var/lock
 exec 9>/var/lock/lg6851f-radio-control.lock
 if ! flock -n 9; then
  if ! /usr/bin/timeout -k 2 12 flock -x 9; then
   vendor_recover_stale_mlo_lock
   if ! flock -n 9; then
    exec 9>&-
    return 1
   fi
  fi
 fi
 trap 'vendor_control_unlock' EXIT
}
vendor_locked_exec() {
 local label="$1" rc
 shift
 # No child may inherit fd 9: an orphaned SDK command must never retain the
 # radio lock after netifd gives up waiting for this handler.
 /usr/bin/timeout -k 1 8 "$@" 9>&-
 rc=$?
 case "$rc" in
  124|137|143) logger -t vendor618 "TIMEOUT $label $1" ;;
 esac
 return "$rc"
}
vendor_mwctl() {
 vendor_locked_exec mwctl /usr/sbin/lg6851f-mwctl "$@"
}
vendor_auth_ctl() {
 vendor_locked_exec hostapd-ctl /usr/sbin/lg6851f-hostapd-ctl "$@"
}
vendor_stop_legacy() {
 local pid n=0
 pid=$(cat "$pidfile" 2>/dev/null)
 case "$pid" in ''|*[!0-9]*) return 0;; esac
 if [ "$(readlink -f /proc/$pid/exe | sed 's/ (deleted)$//')" = "$(readlink -f /usr/sbin/hostapd)" ] &&
    tr '\000' ' ' < /proc/$pid/cmdline | grep -Fq "$conf"; then
  kill "$pid" 2>/dev/null || true
  while kill -0 "$pid" 2>/dev/null && [ "$n" -lt 2 ]; do
   sleep 1; n=$((n+1))
  done
  kill -0 "$pid" 2>/dev/null && return 1
 fi
 rm -f "$pidfile"
 return 0
}
vendor_auth_ready() {
 local n=0
 [ "$(vendor_auth_ctl global PING 2>/dev/null)" = PONG ] && return 0
 /etc/init.d/lg6851f_hostapd start 9>&- || return 1
 while [ "$n" -lt 20 ]; do
  [ "$(vendor_auth_ctl global PING 2>/dev/null)" = PONG ] && return 0
  sleep 1; n=$((n+1))
 done
 return 1
}
vendor_bss_running() {
 vendor_auth_ctl "$vif" STATUS 2>/dev/null | grep -Eq "^state=(ENABLED|DFS|ACS|HT_SCAN|COUNTRY_UPDATE)$"
}
vendor_stop() {
 # Keep the bounded native object: global REMOVE destroys shared PHY state.
 # DISABLE tears down this BSS's sessions/security and MLD membership only.
 local registered
 vendor_stop_changed=0
 registered=$(vendor_auth_ctl global INTERFACES 2>/dev/null) || {
  vendor_link_is_up && return 1
  vendor_stop_legacy; return $?
 }
 if printf '%s\n' "$registered" | grep -Fxq "$vif"; then
  if ! vendor_auth_ctl "$vif" STATUS 2>/dev/null | grep -q '^state=DISABLED$'; then
   vendor_generation changing || return 1
   vendor_auth_ctl "$vif" DISABLE >/dev/null || return 1
  fi
  vendor_stop_changed=1
 fi
 vendor_stop_legacy
}
vendor_disable_unwanted() {
 vendor_stop || return 1
 # Retain the fixed netdev identity down, ready for a future AP in this slot.
 # No broadcast, station membership, or bridge forwarding remains when down.
 if [ -e "/sys/class/net/$vif" ]; then
  ip link set "$vif" down || return 1
 fi
 rm -f "$conf.applied" "$conf.protocol"
}

vendor_acl() {
 local policy=0 entry
 case "$macfilter" in allow) policy=1;; deny) policy=2;; esac
 vendor_mwctl "$vif" set ACLClearAll=1 || return 1
 for entry in $maclist; do
  vendor_mwctl "$vif" set "ACLAddEntry=$entry" || return 1
 done
 vendor_mwctl "$vif" set "AccessPolicy=$policy"
}
vendor_setup_vif() {
 local name="$1" uci_section="$3" ssid key mode network size pid hidden isolate wmm dtim_period macfilter maclist hairpin vendor_protocol
 local hostapd_cfg='' fingerprint actual global_country mlo_group mld_applied mlo_changed=0 n=0 ifname
 vendor_bss_identity "$2" || { failed=INVALID_BSS_SLOT; return; }
 vendor_ensure_bss || { failed=BSS_INTERFACE_CREATE_FAILED; return; }
 ifname="$vif"
 count=$((count+1))
 json_select config
 json_get_vars ssid key mode hidden isolate wmm dtim_period macfilter
 vendor_protocol="$5"
 json_get_values maclist maclist
 json_get_values network network
 # Use the prevalidated transaction, not a second live UCI lookup.
 mlo_group="$4"
 case "$mlo_group" in [0-9]|1[0-6]) ;; *) json_select ..; failed=MLO_CONFIGURATION_MISMATCH; return;; esac
 if [ "$mode" != ap ] || [ "$network" != lan ]; then
  json_select ..; failed=AP_LAN_REQUIRED; return
 fi
 case "${vendor_protocol:-}" in
  '')
   case "$vendor_mode" in
    22|23) vendor_protocol=be;; 16|17) vendor_protocol=ax;;
    14) vendor_protocol=ac;; 8|9) vendor_protocol=n;; *) vendor_protocol=ax;;
   esac
   ;;
  be)
   case "$vendor_mode" in 22|23) ;; *) json_select ..; failed=BE_REQUIRES_EHT_RADIO; return;; esac
   ;;
  ac)
   [ "$expected_band" = 5g ] || { json_select ..; failed=AC_REQUIRES_5G; return; }
   ;;
  ax|n) ;;
  *) json_select ..; failed=INVALID_VENDOR_PROTOCOL; return;;
 esac
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
  fingerprint=$(printf %s "$data" | ucode /usr/share/ucode/lg6851f/bss-fingerprint.uc "$__netifd_device" "$name" "$mlo_group") || {
  failed=CONFIG_FINGERPRINT_FAILED; return;
 }
 fingerprint=$(printf %s "$fingerprint" | sha256sum | cut -d' ' -f1)
 {
  printf 'interface=%s\nbridge=br-lan\nchannel=%s\nieee80211n=1\n' "$vif" "$start_channel"
  # mt_wifi maintains QBSS load with WMM/RRM and owns EHT/channel/MLO.
  # A second hostapd load timer rewrites security/PMF every six seconds.
  printf 'driver_params=vendor_channel_owner=1\n'
  printf '%s\n' "$hostapd_cfg" | sed '/^bss_load_update_period=/d'
  printf 'bss_load_update_period=0\n'
  case "$vendor_protocol" in
   be) printf 'ieee80211ac=1\nieee80211ax=1\nieee80211be=1\ndisable_11ac=0\ndisable_11ax=0\ndisable_11be=0\n';;
   ax)
    if [ "$expected_band" = 5g ]; then printf 'ieee80211ac=1\n'; else printf 'ieee80211ac=0\n'; fi
    printf 'ieee80211ax=1\nieee80211be=0\ndisable_11ac=0\ndisable_11ax=0\ndisable_11be=1\n'
    ;;
   ac) printf 'ieee80211ac=1\nieee80211ax=0\nieee80211be=0\ndisable_11ac=0\ndisable_11ax=1\ndisable_11be=1\n';;
   n) printf 'ieee80211ac=0\nieee80211ax=0\nieee80211be=0\ndisable_11ac=1\ndisable_11ax=1\ndisable_11be=1\n';;
  esac
  printf 'wmm_enabled=%s\ndtim_period=%s\n# vendor618-config=%s\n' "$wmm" "$dtim_period" "$fingerprint"
 } >> "$conf.new"
 pid=$(cat "$pidfile" 2>/dev/null)
 case "$pid" in ''|*[!0-9]*) pid=999999;; esac
 # The stable fingerprint covers all effective setup/security inputs. Generated
 # runtime decorations must not turn a power-only save into AP teardown.
 if [ "$(cat "$conf.applied" 2>/dev/null)" != "$fingerprint" ] ||
    [ "$(cat "$conf.protocol" 2>/dev/null)" != "$vendor_protocol" ] ||
    ! vendor_bss_running || ! vendor_link_is_up; then
  vendor_auth_ready || { failed=NATIVE_AUTH_UNAVAILABLE; return; }
  vendor_stop || { failed=HOSTAPD_STOP_TIMEOUT; return; }
  mv "$conf.new" "$conf"
  if [ "$2" = 0 ]; then ln -sf "$conf" "/var/run/hostapd-$phy.conf"
  else ln -sf "$conf" "/var/run/hostapd-$vif.conf"; fi
  # stop_ap clears vendor PHY state; force ndo_open to restore it.
  ip link set "$vif" down || { failed=LINK_DOWN_FAILED; return; }
  ip link set "$vif" up || { failed=LINK_UP_FAILED; return; }
  global_country=$(iw reg get | sed -n '/^global$/,/^phy#/s/^country \([A-Z0-9][A-Z0-9]\):.*/\1/p' | head -n 1)
  if [ "$global_country" != "$country" ]; then
   iw reg set "$country" || { failed=REGULATORY_FAILED; return; }
  fi
  vendor_mwctl "$phy" set country "code=$country" || { failed=COUNTRY_FAILED; return; }
  ip link set "$vif" master br-lan || { failed=BRIDGE_FAILED; return; }
  vendor_acl || { failed=MAC_ACL_FAILED; return; }
  vendor_mwctl "$vif" set Debug=0 || true
  # Reuse a disabled object after atomically replacing its validated config.
  # The daemon refuses reuse of an active or differently named interface.
  if vendor_auth_ctl global INTERFACES 2>/dev/null | grep -Fxq "$vif"; then
   if ! vendor_auth_ctl "$vif" LG6851F_REUSE "$conf" >/dev/null; then
    # A DISABLE after AP+STA coexistence can leave a dormant single-BSS
    # object which refuses config transplant but can still be restarted.
    # Permit that recovery only for an unchanged effective configuration.
    if [ "$(cat "$conf.applied" 2>/dev/null)" = "$fingerprint" ] &&
       [ "$(cat "$conf.protocol" 2>/dev/null)" = "$vendor_protocol" ] &&
       vendor_auth_ctl "$vif" ENABLE >/dev/null; then
     logger -t vendor618 "$__netifd_device/$vif: re-enabled unchanged dormant BSS"
    else
     failed=HOSTAPD_REUSE_FAILED
     return
    fi
   fi
  else
   vendor_auth_ctl global ADD "$vif" "config=$conf" >/dev/null || { failed=HOSTAPD_FAILED; return; }
  fi
  if ! vendor_bss_running; then
   vendor_auth_ctl "$vif" ENABLE >/dev/null || { failed=HOSTAPD_ENABLE_FAILED; return; }
  fi
  # Never register an AP whose security/beacon setup was rejected.
  vendor_bss_running || { failed=HOSTAPD_NOT_ACTIVE; return; }
  vendor_mwctl "$vif" set phymode "$vendor_mode" || { failed=PHY_MODE_FAILED; return; }
  vendor_acl || { failed=MAC_ACL_FAILED; return; }
  case "$vendor_mode" in
   22|23)
    mld_applied=$(cat "$conf.mld_applied" 2>/dev/null)
    if [ "$mlo_group" != 0 ]; then
     if vendor_mwctl "$vif" set ap_mld index "$mlo_group"; then
      [ "$mld_applied" = "$mlo_group" ] || mlo_changed=1
      mld_applied=$mlo_group
     elif vendor_mwctl "$vif" set ap_mld index 0; then
      # Keep an otherwise valid EHT BSS available when MLO itself fails.
       [ "$mld_applied" = 0 ] || mlo_changed=1
      mld_applied=0
      logger -t vendor618 "$__netifd_device/$vif: MLO fallback to single link"
     else
      failed=MLO_APPLY_FAILED; return
     fi
    else
     case "$mld_applied" in
      ''|0) ;;
      *) if vendor_mwctl "$vif" set ap_mld index 0; then
          mlo_changed=1
          mld_applied=0
         else
          failed=MLO_APPLY_FAILED; return
         fi
       ;;
     esac
    fi
    printf '%s\n' "$mld_applied" > "$conf.mld_applied"
   ;;
  esac
  if [ "$mlo_changed" = 1 ]; then
   # Only an actual MLD group change needs the global two-band metadata pass.
   # Sending this on ordinary SSID add/delete reinitialises unrelated links.
   vendor_auth_ctl global LG6851F_MLO_SYNC >/dev/null || {
    failed=MLO_AUTH_SYNC_FAILED; return;
   }
  fi
  vendor_mwctl "$vif" set "NoForwarding=$isolate" || { failed=ISOLATION_FAILED; return; }
  vendor_mwctl "$vif" set hide_ssid "$hidden" || { failed=HIDDEN_SSID_FAILED; return; }
  vendor_mwctl "$vif" set dtim_int "$dtim_period" || { failed=DTIM_FAILED; return; }
  vendor_mwctl "$vif" set "WmmCapable=$wmm" || { failed=WMM_FAILED; return; }
  vendor_mwctl "$vif" set "RTSThreshold=${rts:-2347}" || { failed=RTS_FAILED; return; }
  vendor_mwctl "$vif" set "FragThreshold=${frag:-2346}" || { failed=FRAG_FAILED; return; }
  vendor_mwctl "$vif" set "HtBssCoex=$((1-${noscan:-0}))" || { failed=COEX_FAILED; return; }
  vendor_mwctl "$vif" set "HtLdpc=${ldpc:-1}" || { failed=LDPC_FAILED; return; }
  # Like the AX6000 reference, leave MCS/NSS/GI selection to vendor RA.
  # 33 is the profile AUTO sentinel, not HE MCS 33. Re-arm on full AP setup
  # only: power-only saves must not reset rate adaptation. Keep the chosen PHY and
  # the MT7990 calibration/aggregation capabilities intact.
  if ! vendor_mwctl "$vif" set HtMcs=33 ||
     ! vendor_mwctl "$vif" set AutoRatePerBSS=1-15; then
   logger -t vendor618 "$__netifd_device: could not restore vendor automatic rate control"
  fi
  case "$distance" in ''|auto) ;; *) vendor_mwctl "$vif" set "Distance=$distance" || { failed=DISTANCE_FAILED; return; };; esac
 else
  rm -f "$conf.new"
 fi
 printf '%s\n' "$fingerprint" > "$conf.applied"
 printf '%s\n' "$vendor_protocol" > "$conf.protocol"
 wireless_add_vif "$name" "$vif"
 # Native intrabss_bridge hands same-BSS forwarding to the bridge. Apply
 # each AP's isolation policy independently, including secondary BSSes.
 if [ -w "/sys/class/net/$vif/brport/hairpin_mode" ]; then
  case "$isolate" in 1|true) hairpin=0;; *) hairpin=1;; esac
  printf '%s' "$hairpin" > "/sys/class/net/$vif/brport/hairpin_mode" || {
   failed=BRIDGE_HAIRPIN_FAILED; return;
  }
 fi
 # The native global authentication daemon belongs to procd, not a radio.
 # procd respawn restores netifd configuration through its bounded supervisor.
 # Restore the current vif expected by for_each_interface.
 json_load "$data"
 json_select interfaces
 json_select "$name"
}
drv_vendor618_setup() {
 vendor_control_lock || {
  # Busy is transient. Preserve netifd's finite retry budget; do not set it to zero.
  echo 'Device setup deferred: RADIO_CONTROL_BUSY'
  return 1
 }
 vendor_identity "$1" || { wireless_setup_failed UNKNOWN_RADIO; return; }
 [ -e /tmp/lg6851f-vendor618-ready ] && [ -e /sys/class/net/$vif ] || {
  wireless_setup_failed VENDOR_MODULES_NOT_READY; return;
 }
  local band country htmode width vendor_mode start_channel ext_chan txpower rts frag distance noscan rxldpc ldpc count=0 failed actual keys
 json_select config
  json_get_vars band channel country htmode txpower rts frag distance noscan rxldpc ldpc vendor_power_level vendor_power_boost
 json_select ..
 country=$(printf %s "${country:-CN}" | tr a-z A-Z)
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
  local client64_layout layout slot section mapped_if frozen_group runtime_id frozen_protocol vendor_generation_id= total=0 started=0 radio_changed=0 radio_fingerprint= power_changed=0 power_fingerprint= radio_slot= radio_vif= client_actual=
 layout=$(printf %s "$data" | ucode /usr/share/ucode/lg6851f/bss-prepare.uc "$__netifd_device") || {
  logger -t vendor618 "$__netifd_device: BSS_PREFLIGHT_DEFERRED"
  return 1
 }
  client64_layout=$(printf %s "$data" | ucode /usr/share/ucode/lg6851f/client64-plan.uc "$__netifd_device") || { logger -t vendor618 "client64: invalid independent uplink; APs retained";client64_layout=; }
  radio_fingerprint=$(printf '%s\n' "$band|$start_channel|$width|$ext_chan|$htmode|$country" | sha256sum | cut -d' ' -f1)
  if [ "$(cat "/tmp/lg6851f-vendor618/$__netifd_device.radio-applied" 2>/dev/null)" != "$radio_fingerprint" ]; then
   radio_changed=1
  fi
  # A STA may retune the shared PHY to the upstream AP's channel. Once the STA
  # disappears, restore the configured AP channel even though the saved radio
  # fingerprint itself did not change.
  if [ -z "$client64_layout" ] && [ -e "/sys/class/net/$vif" ]; then
   client_actual=$(iw dev "$vif" info 2>/dev/null | sed -n 's/.*channel \([0-9]*\) .*/\1/p')
   if [ -n "$client_actual" ] && [ "$client_actual" != "$start_channel" ]; then
    radio_changed=1
   fi
  fi
  power_fingerprint=$(printf '%s\n' "${txpower:-auto}|${vendor_power_level:-1}|${vendor_power_boost:-0}" | sha256sum | cut -d' ' -f1)
  if [ "$(cat "/tmp/lg6851f-vendor618/$__netifd_device.power-applied" 2>/dev/null)" != "$power_fingerprint" ]; then
   power_changed=1
  fi
 # No deletion/stop may run after surviving APs have been restarted.
 vendor_prepare_reconfigure || {
  logger -t vendor618 "$__netifd_device: BSS_PREPARE_DEFERRED"
  return 1
 }
 total=0
 while IFS=" " read -r slot section mapped_if frozen_group runtime_id frozen_protocol; do
  [ -n "$section" ] || continue
  total=$((total+1));failed=
  if [ -z "$radio_slot" ]; then radio_slot="$slot";radio_vif="$mapped_if";fi
  # Runtime IDs and MLO groups belong to the same validated snapshot.
  json_load "$data";json_select interfaces
  json_select "$runtime_id" || { logger -t vendor618 "$__netifd_device/$section: runtime mapping missing";return 1; }
  __cur_interface=$runtime_id
  vendor_setup_vif "$runtime_id" "$slot" "$section" "$frozen_group" "$frozen_protocol"
  if [ -n "$failed" ]; then
   logger -t vendor618 "$__netifd_device/$section ($mapped_if): $failed"
    # A failed AP is isolated to its own native BSS. Never tear down an
    # unrelated SSID or the other radio to recover from this VIF's error.
    vendor_bss_identity "$slot"
    vendor_disable_unwanted || true
   return 1
  else started=$((started+1));fi
 done <<EOF
$layout
EOF
  if [ -n "$radio_slot" ]; then vendor_bss_identity "$radio_slot" || return 1; else vendor_bss_identity 0 || return 1; fi
 if [ "$started" = 0 ]; then
  if [ "$total" = 0 ]; then vendor64_client_apply || true;vendor_generation ready || logger -t vendor618 "$__netifd_device: generation publish failed";wireless_set_data "phy=$phy";wireless_set_up;return;fi
  vendor_band_failed NO_BSS_STARTED; return 1
 fi
 # The radio channel/ACS and power are shared: configure each only once.
 if [ "$radio_changed" = 1 ]; then
 vendor_mwctl "$phy" set channel "num=$start_channel" "bw=$width" "ext_chan=$ext_chan" || {
  vendor_band_failed CHANNEL_FAILED; return 1;
 }
 if [ "$auto_channel" = 1 ]; then vendor_mwctl "$vif" set AutoChannelSel=3 || {
  vendor_band_failed ACS_FAILED; return 1;
 };fi
 fi
 local n=0
 while [ "$n" -lt 10 ]; do
  actual=$(iw dev "$vif" info | sed -n 's/.*channel \([0-9]*\) .*/\1/p')
  [ "$actual" = "$start_channel" ] && break
  [ "$auto_channel" != 1 ] || break
  sleep 1; n=$((n+1))
 done
 if [ "$auto_channel" != 1 ] && [ "$actual" != "$start_channel" ]; then
  vendor_band_failed CHANNEL_NOT_APPLIED; return 1
 fi
  if [ "$radio_changed" = 1 ] || [ "$power_changed" = 1 ]; then
   # BSS-only changes must not rewrite shared radio power.
   vendor_mwctl "$vif" set pwr TxPower=100 || { vendor_band_failed TXPOWER_PERCENT_FAILED; return 1; }
   vendor_mwctl "$vif" set pwr DecreasePower=0 || { vendor_band_failed TXPOWER_FAILED; return 1; }
   if ! LG6851F_RADIO_CONTROL_HELD=1 /usr/sbin/lg6851f-wifi-power apply "$__netifd_device" auto >/dev/null; then
    # The helper restores the prior compensation on any failed readback.
    # Keep the AP available; the wireless form reports the failure via RPC.
    logger -t vendor618 "$__netifd_device: power target not applied; inspect firmware power status"
   fi
 fi
 # Verify all retained BSSes after channel, security, MLD and power setup.
 if ! vendor_verify_band; then
  logger -t vendor618 "$__netifd_device: BSS_FINAL_STATE_DEFERRED; existing BSS retained"
  return 1
 fi
 # A failed uplink is local to that VIF, never a failed AP band.
 vendor64_client_apply || true
 vendor_generation ready || { logger -t vendor618 "$__netifd_device: generation publish failed";return 1; }
 printf '%s\n' "$radio_fingerprint" > "/tmp/lg6851f-vendor618/$__netifd_device.radio-applied.new" || return 1
 mv -f "/tmp/lg6851f-vendor618/$__netifd_device.radio-applied.new" "/tmp/lg6851f-vendor618/$__netifd_device.radio-applied"
 printf '%s\n' "$power_fingerprint" > "/tmp/lg6851f-vendor618/$__netifd_device.power-applied.new" || return 1
 mv -f "/tmp/lg6851f-vendor618/$__netifd_device.power-applied.new" "/tmp/lg6851f-vendor618/$__netifd_device.power-applied"
 wireless_set_data "phy=$phy"
 # WED IRQs are allocated when the radio opens, after early boot steering.
 [ ! -x /usr/sbin/lg6851f-wifi-steering ] || /usr/sbin/lg6851f-wifi-steering || logger -t vendor618 'Could not apply wireless CPU steering'
 wireless_set_up
 # Wake the existing single sampler; never start a second station reader.
 : > /tmp/lg6851f-wireless.wake
 # A shared authentication respawn can lose a different, unchanged band.
 # One bounded event worker reconciles current netifd intent after this
 # transaction completes; it never waits while holding our driver lock.
 /usr/sbin/lg6851f-auth-recover check </dev/null >/dev/null 2>&1 9>&- &
}
vendor_force_teardown() {
 [ ! -e "/tmp/lg6851f-wireless-force-teardown.all" ] || return 0
 [ ! -e "/tmp/lg6851f-wireless-force-teardown.$__netifd_device" ] || return 0
 [ "$(uci -q get "wireless.$__netifd_device")" = wifi-device ] || return 0
 [ "$(uci -q get "wireless.$__netifd_device.disabled")" != 1 ] || return 0
 return 1
}
drv_vendor618_teardown() {
 vendor_control_lock || return
 vendor_identity "$1" || return
 if vendor_force_teardown; then
  vendor_quiesce_band
  return
 fi
 # Configuration reloads ask netifd to tear down a driver before rebuilding it.
 # Preserve live BSS/APCLI state here; drv_vendor618_setup reconciles only the
 # affected native slots. Explicit wifi up/down/restart and a UCI-disabled radio
 # still follow the full quiesce path.
}
add_driver vendor618
