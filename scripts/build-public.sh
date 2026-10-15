#!/bin/sh
# SPDX-License-Identifier: GPL-2.0-only
set -eu
cd "$(CDPATH= cd -- "$(dirname "$0")/.." && pwd)"
jobs="${JOBS:-4}"
./scripts/prepare-public-build.sh
make -j"$jobs" V=s
# The stock driver port is external to the original OpenWrt package graph.
# Rebuild against the kernel and mac80211/backport exports just produced.
python3 scripts/build-vendor-public.py --jobs "$jobs"
make -j"$jobs" V=s
./scripts/lg6851f_sign_export_boot_rootfs.sh
./scripts/lg6851f_make_signed_a_sysupgrade.sh
echo 'Build pipeline completed. Use A-slot output only; vendor runtime is A-slot specific.'

