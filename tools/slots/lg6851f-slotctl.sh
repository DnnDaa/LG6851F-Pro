#!/bin/sh
# SPDX-License-Identifier: GPL-2.0-only
# Quectel BCAB eMMC layout used by this LG6851F only; no automatic reboot.
set -eu
action="${1:-show}"
[ -b /dev/mmcblk0p1 ] && grep -qx 'PARTNAME=misc' /sys/class/block/mmcblk0p1/uevent &&
 [ "$(cat /sys/class/block/mmcblk0p1/size)" = 1024 ] || { echo 'Wrong misc layout';exit 1; }
[ "$(dd if=/dev/mmcblk0p1 bs=1 skip=2052 count=4 2>/dev/null)" = BCAB ] || { echo 'Unknown bootctrl magic';exit 1; }
read_record() { dd if=/dev/mmcblk0p1 bs=1 skip=2060 count=10 2>/dev/null | hexdump -v -e '1/1 "%02x"'; }
before="$(read_record)"
[ "${#before}" = 20 ] || exit 1
echo "current_record=$before"
case "$action" in show) cat /proc/cmdline;exit 0;; a|b) :;; *) echo 'Usage: slotctl.sh show|a|b [--i-have-backed-up-misc]';exit 1;; esac
[ "${2:-}" = --i-have-backed-up-misc ] || { echo 'Back up your full misc first; explicit flag required';exit 1; }
backup="/tmp/lg6851f-slotctl-before-$$.bin"
dd if=/dev/mmcblk0p1 of="$backup" bs=1 skip=2060 count=10 2>/dev/null
write_target() {
case "$action" in
 a)
  expected="0f00$(printf '%s' "$before" | cut -c5-10)0e$(printf '%s' "$before" | cut -c13-20)"
  printf '\016' | dd of=/dev/mmcblk0p1 bs=1 seek=2065 count=1 conv=notrunc 2>/dev/null || return 1
  printf '\017\000' | dd of=/dev/mmcblk0p1 bs=1 seek=2060 count=2 conv=notrunc 2>/dev/null || return 1
 ;;
 b)
  expected="0e$(printf '%s' "$before" | cut -c3-10)0f00$(printf '%s' "$before" | cut -c15-20)"
  printf '\016' | dd of=/dev/mmcblk0p1 bs=1 seek=2060 count=1 conv=notrunc 2>/dev/null || return 1
  printf '\017\000' | dd of=/dev/mmcblk0p1 bs=1 seek=2065 count=2 conv=notrunc 2>/dev/null || return 1
 ;;
esac
}
if ! write_target; then
 dd if="$backup" of=/dev/mmcblk0p1 bs=1 seek=2060 count=10 conv=notrunc 2>/dev/null || true
 sync
 echo 'Write failed; attempted restore. Do not reboot; inspect original misc backup.' >&2
 exit 1
fi
sync
after="$(read_record)"
if [ "$after" != "$expected" ]; then
 dd if="$backup" of=/dev/mmcblk0p1 bs=1 seek=2060 count=10 conv=notrunc 2>/dev/null
 sync
 echo 'Readback mismatch; attempted original record restore. Do not reboot; inspect backup.' >&2
 exit 1
fi
echo "verified_record=$after target=$action backup=$backup"
echo 'No reboot performed. Reboot only after checking the target slot images.'
