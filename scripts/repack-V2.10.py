#!/usr/bin/env python3
"""Repack the verified V2.10 images. Never access a router or block device."""
from pathlib import Path
import argparse,gzip,hashlib,io,json,tarfile,zipfile
p=argparse.ArgumentParser();p.add_argument('--out',required=True);args=p.parse_args()
root=Path(__file__).resolve().parents[1];out=Path(args.out).resolve();out.mkdir(parents=True,exist_ok=True)
meta=json.loads((root/'build-config/V2.10-runtime-inputs.json').read_text());runtime=root/'release-runtime'
sha=lambda b:hashlib.sha256(b).hexdigest()
for name,expected in meta['sha256'].items():assert sha((runtime/name).read_bytes())==expected,name
for slot in ['a','b']:
 blobs={f'boot_{slot}.img':(runtime/f'boot_{slot}.img').read_bytes(),f'connsys_gnss_{slot}.img':(runtime/'connsys_gnss.img').read_bytes()}
 data=(runtime/f'public-{slot}-rootfs.squashfs').read_bytes();assert len(data)<134217728
 blobs[f'rootfs_{slot}.img']=data+bytes(134217728-len(data))
 manifest=json.loads((root/f'build-config/V2.10-manifest-{slot.upper()}.json').read_text())
 for name,b in blobs.items():assert sha(b)==manifest['images'][name]['sha256'],name
 control=f'format=lg6851f-mt6990-{slot}-signed-v1\nslot={slot}\nboot_partition=boot_{slot}\nrootfs_partition=rootfs_{slot}\nconnsys_gnss_partition=connsys_gnss_{slot}\n'
 for name in ['boot','rootfs','connsys_gnss']:
  b=blobs[f'{name}_{slot}.img'];control+=f'{name}_size={len(b)}\n{name}_sha256={sha(b)}\n'
 control+=f'touches_bootctrl=priority-and-{slot}-retry\ntouches_slot_{"b" if slot=="a" else "a"}=0\n'
 sums=''.join(f'{sha(b)}  {n}\n' for n,b in blobs.items())
 common={'CONTROL':control.encode(),'SHA256SUMS':sums.encode(),'signing-manifest.json':json.dumps(manifest,ensure_ascii=False,indent=2).encode()}
 stem=f'lg6851f-pro-V2.10-public-{slot.upper()}'
 with (out/f'{stem}-sysupgrade.tar.gz').open('wb') as f,gzip.GzipFile(fileobj=f,mode='wb',mtime=0,compresslevel=9) as g,tarfile.open(fileobj=g,mode='w|') as t:
  for n,b in {**common,**blobs}.items():m=tarfile.TarInfo(n);m.mode=0o644;m.size=len(b);t.addfile(m,io.BytesIO(b))
 with zipfile.ZipFile(out/f'{stem}-mtk.zip','w',zipfile.ZIP_DEFLATED,compresslevel=9) as z:
  for n,b in blobs.items():z.writestr('download_package/'+n,b)
  z.writestr('download_package/SHA256SUMS.txt',sums)
  z.writestr('download_package/LG6851F_scatter.xml',(runtime/f'scatter-{slot}.xml').read_bytes())
  z.writestr('firmware-manifest.json',json.dumps(manifest,ensure_ascii=False,indent=2))
  for f in (runtime/f'download_agent-{slot}').rglob('*'):
   if f.is_file():z.write(f,'download_package/download_agent/'+f.relative_to(runtime/f'download_agent-{slot}').as_posix())
(out/'SHA256SUMS.txt').write_text(''.join(f'{sha(f.read_bytes())}  {f.name}\n' for f in sorted(out.iterdir()) if f.is_file() and f.name!='SHA256SUMS.txt'))
print('A/B packages ready:',out)
