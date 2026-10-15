#!/usr/bin/env python3
"""Patch an owner's LG6851F stock SquashFS offline. Never flash a device."""
from pathlib import Path
import argparse,subprocess,hashlib,json,os

p=argparse.ArgumentParser()
p.add_argument('stock_rootfs',type=Path)
p.add_argument('--work-dir',type=Path,required=True)
a=p.parse_args();source=a.stock_rootfs.resolve();work=a.work_dir.resolve()
assert source.is_file() and source.stat().st_size==134217728,'Expected own128MiB LG6851F rootfs image'
assert source.read_bytes()[:4]==b'hsqs','Expected direct eMMC SquashFS, not NAND/UBI'
assert not work.exists(),'Use a new private work directory'
work.mkdir(parents=True)
tree=work/'rootfs'
subprocess.run(['unsquashfs','-no-progress','-d',str(tree),str(source)],check=True)
rc=tree/'etc/init.d/rcS';text=rc.read_text()
assert 'fhdata_init.sh' in text and 'fhdrv_net_init.sh' in text and 'fac_auto_check_hardware_LG6851F.sh' in text,'Unrecognized stock boot chain'
assert 'lg6851f-owner-ssh' not in text,'Image already patched'
# Do not assume stock rcS executes /etc/rc.d. Preserve all existing startup code.
text+='''
# lg6851f-owner-ssh: bounded, optional owner-provided bootstrap; no overlay changes.
(
    n=0
    while [ "$n" -lt 120 ]; do
        if [ -s /data/lg6851f-stock-ssh-bootstrap.sh ]; then
            sh /data/lg6851f-stock-ssh-bootstrap.sh && exit 0
        fi
        n=$((n + 1))
        sleep 1
    done
) >/tmp/lg6851f-owner-ssh.log 2>&1 &
'''
rc.write_text(text);subprocess.run(['sh','-n',str(rc)],check=True)
dest=work/'rootfs-owner-ssh.img'
subprocess.run(['mksquashfs',str(tree),str(dest),'-comp','xz','-b','262144','-no-xattrs','-noappend','-no-progress'],check=True)
assert dest.stat().st_size<=source.stat().st_size
with dest.open('ab') as f:f.truncate(source.stat().st_size)
def digest(path):
 h=hashlib.sha256()
 with path.open('rb') as f:
  for chunk in iter(lambda:f.read(1024*1024),b''):h.update(chunk)
 return h.hexdigest()
(work/'patch-record.json').write_text(json.dumps({'input_sha256':digest(source),'output_sha256':digest(dest),'changed':['etc/init.d/rcS'],'block_device_writes':False,'physical_boot_tested':False},indent=2)+'\n')
print(dest)
print('Offline patched only. Flash the matching STOCK slot rootfs with its original boot; never pair it with an OpenWrt boot.')

