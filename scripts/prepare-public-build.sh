#!/bin/sh
# SPDX-License-Identifier: GPL-2.0-only
set -eu
cd "$(CDPATH= cd -- "$(dirname "$0")/.." && pwd)"
case "$PWD" in *' '*) echo 'Use a Linux, case-sensitive path without spaces.' >&2; exit 1;; esac
: > feeds.conf
for name in packages luci routing telephony video; do
 [ ! -d "feeds-snapshot/$name" ] || printf 'src-link %s %s/feeds-snapshot/%s\n' "$name" "$PWD" "$name" >> feeds.conf
done
./scripts/feeds update -a
./scripts/feeds install -a
if [ ! -f private-key.pem ]; then
 umask 077
 openssl genpkey -algorithm EC -pkeyopt ec_paramgen_curve:prime256v1 -out private-key.pem
 openssl pkey -in private-key.pem -pubout -out public-key.pem
fi
make defconfig
echo 'Pinned local feeds installed. APK signing key is local, never commit private-key.pem.'

