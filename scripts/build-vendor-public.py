#!/usr/bin/env python3
from pathlib import Path
import argparse,subprocess,shutil,os
p=argparse.ArgumentParser();p.add_argument('--jobs',default='4');a=p.parse_args()
r=Path(__file__).resolve().parent.parent
k=next(r.glob('build_dir/target-*/linux-mediatek_mt6990/linux-6.18.44'))
tool=next(r.glob('staging_dir/toolchain-aarch64_cortex-a55*/bin/aarch64-openwrt-linux-musl-gcc'))
prefix=str(tool)[:-3];strip=prefix+'strip'
for part in ['coex','cmn','warp','wifi','hwifi']:
 subprocess.run(['python3',str(r/'scripts/vendor-build-part.py'),part],check=True)
dest=r/'files/usr/lib/lg6851f-vendor618/modules';dest.mkdir(parents=True,exist_ok=True)
for rel in ['mt_wifi_cmn','wifi/mt_wifi_ap','wifi/wlan_hwifi','wifi/warp_driver/warp','wifi_md_coex']:
 for ko in (r/'vendor-source'/rel).glob('*.ko'):
  if ko.stem=='mt7990_dbg':continue
  target=dest/ko.name;shutil.copyfile(ko,target);subprocess.run([strip,'--strip-debug',str(target)],check=True)
mods=r/'build-public-modules';mods.mkdir(exist_ok=True)
for name in ['lg6851f_cpufreq_hw','lg6851f_panel_leds','lg6851f_ppe_stats']:shutil.copyfile(r/'drivers'/(name+'.c'),mods/(name+'.c'))
hnats=list(r.glob('build_dir/target-*/**/mt6990-hwnat-*/hwnat.ko'))
if len(hnats)!=1:raise RuntimeError('Expected one freshly built MT6990 HNAT provider')
provider=hnats[0].parent
symvers=provider/'Module.symvers'
if not symvers.exists():raise RuntimeError('Build HNAT first; PPE observer requires its exported symbol versions')
flags=['CONFIG_RA_NAT_HW', 'CONFIG_RA_HW_NAT', 'CONFIG_RA_HW_NAT_TBL_32K', 'CONFIG_RA_HW_NAT_HASH1', 'CONFIG_HW_NAT_AUTO_MODE', 'CONFIG_RA_HW_NAT_WIFI', 'CONFIG_RA_HW_NAT_WIFI_NEW_ARCH', 'CONFIG_ARCH_MT7622_WIFI_HW_NAT', 'CONFIG_PPE_MIB', 'CONFIG_ARCH_COLGIN', 'CONFIG_RA_HW_NAT_IPV6', 'CONFIG_SUPPORT_OPENWRT', 'CONFIG_QDMA_SUPPORT_QOS', 'CONFIG_HW_NAT_SW_DVFS', 'CONFIG_HNAT_V2', 'CONFIG_MTK_TINYSYS_MEDMCU_SUPPORT']
(mods/'Makefile').write_text('obj-m += lg6851f_cpufreq_hw.o lg6851f_panel_leds.o lg6851f_ppe_stats.o\n'
 + 'ccflags-y += -Werror -I'+str(provider/'hw_nat')+' -I'+str(provider/'include')+'\n'
 + 'ccflags-y += -I$(srctree)/drivers/misc/mediatek/medmcu/rv33 -I$(srctree)/drivers/misc/mediatek/nat46_util\n'
 + 'ccflags-y += '+' '.join('-D'+f+'=1' for f in flags)+'\n')
subprocess.run(['make','-C',str(k),'M='+str(mods),'ARCH=arm64','CROSS_COMPILE='+prefix,'KBUILD_EXTRA_SYMBOLS='+str(symvers),'-j'+a.jobs,'modules'],env={**os.environ,'STAGING_DIR':str(r/'staging_dir')},check=True)
targetdir=r/'files/lib/modules/6.18.44';targetdir.mkdir(parents=True,exist_ok=True)
for name in ['lg6851f_cpufreq_hw','lg6851f_panel_leds','lg6851f_ppe_stats']:
 target=targetdir/(name+'.ko');shutil.copyfile(mods/(name+'.ko'),target);subprocess.run([strip,'--strip-debug',str(target)],check=True)
hnats=list(r.glob('build_dir/target-*/**/mt6990-hwnat-*/hwnat.ko'))
if len(hnats)!=1:raise RuntimeError('Expected freshly compiled hwnat.ko; inspect build_dir and do not reuse an unrelated kernel module')
shutil.copyfile(hnats[0],targetdir/'hwnat.ko');subprocess.run([strip,'--strip-debug',str(targetdir/'hwnat.ko')],check=True)
