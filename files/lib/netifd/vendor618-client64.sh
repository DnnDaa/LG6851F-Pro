# Client-only hooks for the old 6.18.44 vendor driver. No AP error propagation.
vendor64_identity() {
 case "$__netifd_device" in radio0) c64_if=apcli0;;radio1) c64_if=apclii0;;*) return 1;;esac
 c64_conf="/tmp/lg6851f-vendor618/$c64_if.conf"
 c64_pid="/tmp/lg6851f-vendor618/$c64_if.pid"
}
vendor64_owned() {
 local p="$1"
 case "$p" in ''|*[!0-9]*)return 1;;esac
 [ "$(readlink -f "/proc/$p/exe")" = "$(readlink -f /usr/sbin/wpa_supplicant)" ] || return 1
 tr '\000' '\n' <"/proc/$p/cmdline" 2>/dev/null | grep -Fxq "$c64_conf"
}
vendor64_existing() {
 local candidate found= count=0
 c64_actual=
 for candidate in "$c64_if" "${c64_if}0"; do
  [ -e "/sys/class/net/$candidate" ] || continue
  [ "$(basename "$(readlink -f "/sys/class/net/$candidate/phy80211")")" = "$phy" ] || return 1
  iw dev "$candidate" info | grep -Eq '^[[:space:]]*type managed$' || return 1
  found="$candidate";count=$((count+1))
 done
 # Never guess between two interfaces or touch another PHY/type.
 [ "$count" -le 1 ] || return 1
 c64_actual="$found"
}
vendor64_client_stop() {
 local p n=0 flags
 vendor64_identity || return 1
 p=$(cat "$c64_pid" 2>/dev/null)
 if vendor64_owned "$p";then
  kill "$p" 2>/dev/null || true
  while vendor64_owned "$p" && [ "$n" -lt 2 ];do sleep 1;n=$((n+1));done
  if vendor64_owned "$p";then kill -KILL "$p" 2>/dev/null || return 1;fi
 fi
 vendor64_existing || return 1
 if [ -n "$c64_actual" ];then
  flags=$(cat "/sys/class/net/$c64_actual/flags") || return 1
  case "$flags" in 0x[0-9a-fA-F]*) ;; *) return 1;;esac
  if [ $((flags & 1)) = 1 ];then ip link set "$c64_actual" down || return 1;fi
 fi
 # Keep the bounded managed object DOWN. No cfg80211 delete/recreate cycle,
 # no underflow of the vendor's dynamic-STA counter on profile-owned netdevs.
 rm -f "$c64_pid" "$c64_conf" "$c64_conf.applied"
}
vendor64_client_prepare() {
 vendor64_existing || return 1
 if [ -z "$c64_actual" ];then
  # Only missing hardware is created; AP-only boots never create a STA.
  ip link set "$vif" up || return 1
  iw phy "$phy" interface add "$c64_if" type managed || { logger -t vendor618 "client64/$c64_if: native interface creation rejected";return 1; }
  vendor64_existing || return 1
 fi
 [ -n "$c64_actual" ] || return 1
 if [ "$c64_actual" != "$c64_if" ];then
  ip link set "$c64_actual" down || return 1
  ip link set "$c64_actual" name "$c64_if" || return 1
 fi
 [ "$(basename "$(readlink -f "/sys/class/net/$c64_if/phy80211")")" = "$phy" ] || return 1
 # The MT7990 vendor APCLI is permanently configured for four-address frames;
 # nl80211 rejects changing it with EOPNOTSUPP (-95). Keeping the profile-owned
 # value is required for AP+STA coexistence and also matches the working
 # MT7986/mtwifi reference.
 iw dev "$c64_if" info 2>/dev/null | grep -q '4addr: on' || {
  logger -t vendor618 "client64/$c64_if: vendor APCLI is not in 4addr mode"
  return 1
 }
}
vendor64_client_start() {
 local id="$1" ifname mode network_bridge= p fp n=0 debug_args= __cur_interface="$1"
 vendor64_identity || return 1
 fp=$(printf %s "$data" | ucode /usr/share/ucode/lg6851f/bss-fingerprint.uc "$__netifd_device" "$id" 0) || return 1
 fp=$(printf %s "$fp" | sha256sum | cut -d' ' -f1) || return 1
 p=$(cat "$c64_pid" 2>/dev/null)
 if [ "$(cat "$c64_conf.applied" 2>/dev/null)" = "$fp" ] && vendor64_owned "$p" && [ -e "/sys/class/net/$c64_if" ];then
  wireless_add_vif "$id" "$c64_if";wireless_add_process "$p" /usr/sbin/wpa_supplicant;return 0
 fi
 vendor64_client_stop || return 1
 [ -x /usr/sbin/wpa_supplicant ] || return 1
 vendor64_client_prepare || return 1
 json_load "$data";json_select interfaces;json_select "$id";json_select config
 ifname="$c64_if";mode=sta
 umask 077
 wpa_supplicant_prepare_interface "$c64_if" nl80211 || return 1
 wpa_supplicant_add_network "$c64_if" "" "$htmode" || return 1
 # The MT7990 vendor driver reports transient low-ACK conditions while its
 # AP and APCLI coexist. Keep the established link until the AP or driver
 # actually disconnects instead of tearing it down from one noisy sample.
 { printf 'disassoc_low_ack=0\n'; cat "$_config"; } > "$_config.low_ack" || return 1
 mv -f "$_config.low_ack" "$_config" || return 1
 mv -f "$_config" "$c64_conf" || return 1
 ip link set "$c64_if" up || return 1
 # Diagnostic logging is opt-in and has no cost in normal operation.
 [ ! -e /tmp/lg6851f-wpa-debug ] || debug_args="-d -f /tmp/lg6851f-wpa-$c64_if.log"
 /usr/sbin/wpa_supplicant -D nl80211 -i "$c64_if" -c "$c64_conf" $debug_args -s </dev/null >/dev/null 2>&1 9>&- &
 p=$!;printf '%s\n' "$p" >"$c64_pid"
 # Integer sleep works on the exact old BusyBox; no fractional-sleep spin.
 while ! vendor64_owned "$p" && [ "$n" -lt 2 ];do kill -0 "$p" 2>/dev/null || return 1;sleep 1;n=$((n+1));done
 vendor64_owned "$p" || return 1
 printf '%s\n' "$fp" >"$c64_conf.applied"
 wireless_add_vif "$id" "$c64_if"
 # Omit the third argument: netifd treats any non-empty value as required.
 # A failed optional uplink therefore cannot tear down unrelated APs.
 wireless_add_process "$p" /usr/sbin/wpa_supplicant
 json_load "$data"
}
vendor64_client_apply() {
 local id __cur_interface
 if [ -z "$client64_layout" ];then vendor64_client_stop;return $?;fi
 while IFS= read -r id;do
  [ -n "$id" ] || continue
  __cur_interface="$id"
  if ! vendor64_client_start "$id";then
   vendor64_client_stop || true
   logger -t vendor618 "client64/$id: setup failed; existing APs retained"
   wireless_setup_vif_failed CLIENT_SETUP_FAILED
   json_load "$data"
   return 1
  fi
 done <<EOF
$client64_layout
EOF
}
