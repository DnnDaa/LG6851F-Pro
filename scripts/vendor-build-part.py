from pathlib import Path
import os,subprocess,json,sys

base=Path(__file__).resolve().parent.parent
ow=base
k=next(ow.glob('build_dir/target-*/linux-mediatek_mt6990/linux-6.18.44'))
st=next(ow.glob('staging_dir/target-aarch64_cortex-a55*'))
tc=next(ow.glob('staging_dir/toolchain-aarch64_cortex-a55*/bin/aarch64-openwrt-linux-musl-gcc'));tc=str(tc)[:-3]
part=sys.argv[1]
paths={'cmn':base/'vendor-source/mt_wifi_cmn','wifi':base/'vendor-source/wifi/mt_wifi_ap','hwifi':base/'vendor-source/wifi/wlan_hwifi','warp':base/'vendor-source/wifi/warp_driver/warp','coex':base/'vendor-source/wifi_md_coex'}
args=['make','-C',str(k),'-j8','ARCH=arm64','CROSS_COMPILE='+str(tc),'M='+str(paths[part]),'V=0','CONFIG_WERROR=n']
flags=['-I'+str(st/'usr/include/mac80211-backport'),'-I'+str(st/'usr/include/mac80211-backport/uapi'),'-I'+str(st/'usr/include/mac80211'),'-I'+str(st/'usr/include/mac80211/uapi'),'-DBACKPORT_NOSTDINC','-include','backport/backport.h']
extra=[str(k.parent/'symvers/mac80211.symvers')]
if part in ('cmn','wifi'):
 flags+=['-DMT_WIFI_COMMON_SUPPORT']
 if part=='cmn':flags+=['-DRT_CFG80211_SUPPORT','-I'+str(paths['cmn'])]
 else:
  flags+=['-I'+str(base/'vendor-source/wifi'),'-I'+str(base/'vendor-source/wifi/include'),'-I'+str(base/'vendor-include/mtk_nl80211_inc'),'-include',str(base/'vendor-include/vendor618_wext.h')]
  cfg=json.loads((base/'vendor-config.json').read_text())
  args+=['CONFIG_'+key+'='+value for key,value in cfg.items()]
  args+=['CFG_CFG80211_VERSION=6.18.0','LINUX_DIR='+str(k.parent)]
  extra+=[str(paths['cmn']/'Module.symvers'),str(paths['coex']/'Module.symvers'),str(k.parent/'symvers/mt6990-hwnat.symvers')]
elif part=='warp':
 args+=['CONFIG_WARP_VERSION=2','CONFIG_WARP_CHIPSET=mt6990','CONFIG_WARP_WO_EMBEDDED_LOAD=y','CONFIG_WED_HW_RRO_SUPPORT=y','CONFIG_WARP_DBG_SUPPORT=y','PKG_BUILD_DIR='+str(paths['warp'])+'/','CONFIG_ARCH_MEDIATEK=y']
 flags=[];extra=[]
elif part=='hwifi':
 cfg=json.loads((base/'vendor-config.json').read_text())
 args+=['CONFIG_SUPPORT_OPENWRT=y','CONFIG_HWIFI_HIF_WED=m','CONFIG_HWIFI_HIF_NPU=n','CONFIG_HWIFI_MD_COEX_SUPPORT=y']
 for chip in ('MT7915','MT7916','BELLWETHER','MT7992','MT799A','MT7993','MT799B'):
  args+=['CONFIG_HWIFI_CHIP_'+chip+'=n','CONFIG_HWIFI_CHIP_'+chip+'_DBG=n']
 flags+=['-DMT_WIFI_COMMON_SUPPORT']
 extra+=[str(paths['cmn']/'Module.symvers'),str(paths['wifi']/'Module.symvers'),str(paths['warp']/'Module.symvers'),str(paths['coex']/'Module.symvers')]
elif part=='coex':
 args+=['CONFIG_SUPPORT_OPENWRT=y','CONFIG_COEXISTENCE_SUPPORT=m']
 flags=['-I'+str(k/'drivers/misc/mediatek/include'),'-DCOEX_DIRECT_PATH','-DCONFIG_WIFI_MD_COEX_SUPPORT=1']
 extra=[]
args+=['KCFLAGS=-Wno-error -include '+str(base/'vendor-include/vendor618_compat.h'),'NOSTDINC_FLAGS='+' '.join(flags),'KBUILD_EXTRA_SYMBOLS='+' '.join(extra),'modules']
env=dict(os.environ,PATH='/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin',FORCE_UNSAFE_CONFIGURE='1',STAGING_DIR=str(st))
log=base/'build-logs'/(part+'-build.log');log.parent.mkdir(exist_ok=True)
(log.parent/(part+'-command.json')).write_text(json.dumps(args,indent=2)+'\n')
with log.open('w') as f:r=subprocess.run(args,stdout=f,stderr=subprocess.STDOUT,env=env)
print(part,'exit',r.returncode,'log',log)
print('\n'.join(log.read_text(errors='replace').splitlines()[-45:]))
sys.exit(r.returncode)
