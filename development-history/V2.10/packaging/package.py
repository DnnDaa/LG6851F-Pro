from pathlib import Path
import concurrent.futures,copy,gzip,hashlib,io,json,shutil,subprocess,sys,tarfile,xml.etree.ElementTree as ET,zipfile
H=Path(__file__).resolve().parent;R=H.parents[1];BUILD=R/'build/release-v2.10'
OUT=R/'release-assets/V2.10';OUT.mkdir(exist_ok=True)
BASE=R/'build/systemfix-20261029-94-b/mtk/download_package'
sys.path.insert(0,str(R/'tools/ssh-python'))
namespace={'__file__':str(R/'iterations/systemfix-20261022-39/check-signed-boot.py')};src=(R/'iterations/systemfix-20261022-39/check-signed-boot.py').read_text();exec(src[:src.index('original=(ROOT/')],namespace)
sha=lambda b:hashlib.sha256(b).hexdigest()
def js(x):return (json.dumps(x,ensure_ascii=False,indent=2)+'\n').encode()
original=namespace['unpack']((R/'private-device-work/codex-fw-20261019/boot_a.img').read_bytes())
boots={s:(R/'build/systemfix-20261022-39/mtk/download_package/boot_a.img' if s=='a' else BASE/'boot_b.img').read_bytes() for s in ['a','b']}
boot_checks={}
for slot,blob in boots.items():
 parsed=namespace['unpack'](blob);cert1,cert2=parsed[4]
 assert cert1==original[4][0]
 keys=namespace['public_keys'](cert1);namespace['verify'](cert1,keys);_,tbs=namespace['verify'](cert2,keys)
 assert tbs.count(hashlib.sha256(parsed[3][:2048]).digest())==1 and tbs.count(hashlib.sha256(parsed[3][2048:]).digest())==1
 raw=gzip.decompress(parsed[1]);assert sha(raw)=='ff805d39450a04f011cf859cfdddbda7dfcde8b7600ee1bb0bdedbebc7a42f60'
 root=26 if slot=='a' else 39;assert parsed[2].count(f'root=/dev/mmcblk0p{root}'.encode())==1
 boot_checks[slot]={'sha256':sha(blob),'kernel_Image_sha256':sha(raw),'root_partition':f'/dev/mmcblk0p{root}','cert1_identical':True,'cert1_signature_valid':True,'cert2_signature_valid':True,'header_and_body_hashes_valid':True,'android_boot_id_valid':True}
assert namespace['unpack'](boots['a'])[1]==namespace['unpack'](boots['b'])[1]
(H/'boot-checks.json').write_bytes(js(boot_checks))

def compress(pair):
 kind,slot=pair;p=BUILD/f'{kind}-{slot}'
 args=[str(R/'tools/msys/usr/bin/sqfstar.exe'),'-force','-comp','xz','-b','262144','-exports','-no-xattrs','-numeric-owner','-root-mode','0755','-root-uid','0','-root-gid','0','-default-uid','0','-default-gid','0','-processors','2','-mem','384M','-no-progress','-exit-on-error',str(p/'rootfs.squashfs')]
 with (p/'assembled-root.tar').open('rb') as inp,(p/'sqfstar.log').open('wb') as log:subprocess.run(args,stdin=inp,stdout=log,stderr=subprocess.STDOUT,check=True)
 print('ROOTFS_READY',kind,slot,flush=True)
with concurrent.futures.ThreadPoolExecutor(max_workers=3) as executor:list(executor.map(compress,[('private','a'),('public','a'),('public','b')]))

results={}
for kind,slot in [('private','a'),('public','a'),('public','b')]:
 p=BUILD/f'{kind}-{slot}';out=OUT if kind=='public' else R/'release-assets/20261029-systemfix-94-a';out.mkdir(exist_ok=True)
 image_dir=p/'mtk/download_package';image_dir.mkdir(parents=True,exist_ok=True)
 blobs={f'boot_{slot}.img':boots[slot],f'connsys_gnss_{slot}.img':(BASE/'connsys_gnss_b.img').read_bytes()}
 rootfs=(p/'rootfs.squashfs').read_bytes();assert len(rootfs)<134217728
 blobs[f'rootfs_{slot}.img']=rootfs+bytes(134217728-len(rootfs))
 for n,b in blobs.items():(image_dir/n).write_bytes(b)
 scatter=ET.parse(BASE/'LG6851F_scatter.xml');selected=[]
 for part in scatter.getroot().iter('partition_index'):
  name=part.findtext('partition_name');active=name in [f'boot_{slot}',f'rootfs_{slot}',f'connsys_gnss_{slot}']
  part.find('is_download').text='true' if active else 'false';part.find('file_name').text=name+'.img' if active else None
  if active:selected.append({'name':name,'address':part.findtext('linear_start_addr'),'size':int(part.findtext('partition_size'),16)})
 assert len(selected)==3
 scatter.write(image_dir/'LG6851F_scatter.xml',encoding='utf-8',xml_declaration=True)
 shutil.copytree(BASE/'download_agent',image_dir/'download_agent',dirs_exist_ok=True)
 f=ET.parse(image_dir/'download_agent/flash.xml');f.getroot().find('build_version').text=f'LG6851F_{slot.upper()}_V2.10_SYSTEMFIX94_6.18.44';f.write(image_dir/'download_agent/flash.xml',encoding='utf-8',xml_declaration=True)
 sums=''.join(f'{sha(b)}  {n}\n' for n,b in blobs.items())
 (image_dir/'SHA256SUMS.txt').write_text(sums,encoding='utf-8',newline='\n')
 version='V2.10' if kind=='public' else 'systemfix-94-A'
 manifest={'version':version,'source_systemfix':94,'target_slot':slot,'format':f'lg6851f-mt6990-{slot}-signed-v1','kernel':'6.18.44','public_build':kind=='public','source_94_b_sha256':sha((R/'release-assets/20261029-systemfix-94-b/firmware-manifest.json').read_bytes()),'boot_signature_checks':boot_checks[slot],'mtk_selected_partitions':selected,'module_count':120,'modules_unchanged_from_94B':True,'cold_boot_tested':False,'cross_device_tested':False,'device_flashing_performed':False,'other_slot_firmware_written':False,'shared_factory_ssh_written':False,'calibration_policy':'own device read-only fh_data; donor calibration excluded' if kind=='public' else 'original 94 self-use calibration','images':{n:{'bytes':len(b),'sha256':sha(b)} for n,b in blobs.items()},'features':['Wi-Fi 7 / MLO / AP and client','incremental AP create/delete','HNAT / PPE / WED / CPU-to-Wi-Fi','TurboACC / default, UDP fullcone, randomized symmetric NAT','BBR or CUBIC, CAKE independent controls','shared cached status dashboard','dynamic cellular wan6 and LAN IPv6 RA/DHCPv6 server','Chinese UI, CPU/fan/LED, OpenClash/Mihomo and Nikki retained']}
 other='b' if slot=='a' else 'a'
 control=f'format=lg6851f-mt6990-{slot}-signed-v1\nslot={slot}\nboot_partition=boot_{slot}\nrootfs_partition=rootfs_{slot}\nconnsys_gnss_partition=connsys_gnss_{slot}\n'
 for name in ['boot','rootfs','connsys_gnss']:
  b=blobs[f'{name}_{slot}.img'];control+=f'{name}_size={len(b)}\n{name}_sha256={sha(b)}\n'
 control+=f'touches_bootctrl=priority-and-{slot}-retry\ntouches_slot_{other}=0\n'
 stem=f'lg6851f-pro-V2.10-public-{slot.upper()}' if kind=='public' else 'lg6851f-pro-current-selfuse-20261029-systemfix-94-a'
 web=out/f'{stem}-sysupgrade.tar.gz';mtk=out/f'{stem}-mtk.zip'
 with web.open('wb') as output,gzip.GzipFile(fileobj=output,mode='wb',compresslevel=9,mtime=0) as gz,tarfile.open(fileobj=gz,mode='w|',format=tarfile.GNU_FORMAT) as t:
  for n,b in [('CONTROL',control.encode()),('SHA256SUMS',sums.encode()),('signing-manifest.json',js(manifest)),*blobs.items()]:
   m=tarfile.TarInfo(n);m.size=len(b);m.mode=0o644;t.addfile(m,io.BytesIO(b))
 readme=f'LG6851F Pro {version} / {slot.upper()}槽。仅适用T830/MT6990/eMMC。\n网页包仅供已运行同槽OpenWrt且升级器支持该格式时使用；跨槽安装请按对应MTK包教程操作。\nMTK只用Download Only，仅选本包三个镜像，不格式化、不选Firmware Upgrade、不写其他分区。\n本包会覆盖{slot.upper()}槽原系统；保留另一槽固件与共享原厂SSH文件。若A槽仍为原厂救援系统，请用B槽包。\n'+('公开版已清除个人密码、主机密钥、订阅、固定MAC和小区锁定；首启LAN设置root密码，Wi-Fi生成随机密码，从本机fh_data只读加载校准。\n' if kind=='public' else '私人自用配置，仅本地保存，禁止公开上传。\n')+'本组合完成离线检查，未重新刷入冷启动或跨设备验收。\n'
 (p/'mtk/README.txt').write_text(readme,encoding='utf-8',newline='\n');(p/'mtk/firmware-manifest.json').write_bytes(js(manifest))
 with zipfile.ZipFile(mtk,'w',zipfile.ZIP_DEFLATED,compresslevel=9) as z:
  for file in sorted((p/'mtk').rglob('*')):
   if file.is_file():z.write(file,file.relative_to(p/'mtk').as_posix())
 manifest['packages']={f.name:{'bytes':f.stat().st_size,'sha256':sha(f.read_bytes())} for f in [web,mtk]}
 (out/f'firmware-manifest-{slot.upper()}.json').write_bytes(js(manifest));(out/f'README-{slot.upper()}.txt').write_text(readme,encoding='utf-8',newline='\n')
 results[f'{kind}-{slot}']=manifest
 print('PACKAGES_READY',kind,slot,flush=True)
(H/'package-results.json').write_bytes(js(results))
for out in [OUT,R/'release-assets/20261029-systemfix-94-a']:
 files=sorted(f for f in out.iterdir() if f.is_file() and f.name!='SHA256SUMS.txt')
 (out/'SHA256SUMS.txt').write_text(''.join(f'{sha(f.read_bytes())}  {f.name}\n' for f in files),encoding='utf-8',newline='\n')

