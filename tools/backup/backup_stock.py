#!/usr/bin/env python3
"""Read-only SSH backup to the owner's computer. No router files are written."""
import argparse,base64,getpass,hashlib,json,re,os
from pathlib import Path
import paramiko

p=argparse.ArgumentParser();p.add_argument('--host',default='192.168.8.1');p.add_argument('--port',type=int,default=22)
p.add_argument('--output',type=Path,required=True);p.add_argument('--full-disk',action='store_true');a=p.parse_args()
assert not a.output.exists() or not any(a.output.iterdir()),'Use a new or empty private output directory'
a.output.mkdir(parents=True,exist_ok=True)
class VerifyNewHost(paramiko.MissingHostKeyPolicy):
 def missing_host_key(self,client,hostname,key):
  fingerprint='SHA256:'+base64.b64encode(hashlib.sha256(key.asbytes()).digest()).decode().rstrip('=')
  print('First connection:',hostname,key.get_name(),fingerprint)
  if input('Verify this is your CPE; type yes to trust: ').strip()!='yes':raise RuntimeError('Host not trusted')
  client.get_host_keys().add(hostname,key.get_name(),key)
client=paramiko.SSHClient();client.load_system_host_keys();client.set_missing_host_key_policy(VerifyNewHost())
client.connect(a.host,port=a.port,username='root',password=getpass.getpass('Device SSH password: '),look_for_keys=False,allow_agent=False,timeout=15)
def command(cmd):
 _,stdout,stderr=client.exec_command(cmd,timeout=30);data=stdout.read();err=stderr.read();code=stdout.channel.recv_exit_status()
 if code:raise RuntimeError('Read command failed: '+err.decode(errors='replace')[:300])
 return data
assert command('id -u').strip()==b'0'
raw=command("for p in /sys/class/block/mmcblk0p*; do printf '%s ' \"${p##*/}\"; cat \"$p/size\"; cat \"$p/uevent\" | grep '^PARTNAME='; done").decode()
parts=[]
for block in re.finditer(r'(mmcblk0p\d+) (\d+)\s+PARTNAME=([^\r\n]+)',raw):parts.append({'device':block[1],'bytes':int(block[2])*512,'name':block[3]})
assert any(x['device']=='mmcblk0p26' and x['name']=='rootfs_a' and x['bytes']==134217728 for x in parts),'Wrong model/partition layout'
assert any(x['device']=='mmcblk0p44' and x['name']=='fh_data' for x in parts),'Missing factory partition'
manifest={'host':a.host,'cmdline':command('cat /proc/cmdline').decode().strip(),'partition_map':parts,'backups':[],'router_writes':False}
(a.output/'PARTITION_MAP.json').write_text(json.dumps(manifest,indent=2)+'\n',encoding='utf-8')
client.get_host_keys().save(str(a.output/'known_hosts'))
def stream(cmd,filename,expected):
 _,stdout,stderr=client.exec_command(cmd);target=a.output/filename;partial=target.with_suffix(target.suffix+'.partial');h=hashlib.sha256();size=0
 with partial.open('wb') as f:
  while True:
   chunk=stdout.read(1024*1024)
   if not chunk:break
   f.write(chunk);h.update(chunk);size+=len(chunk)
 err=stderr.read();code=stdout.channel.recv_exit_status()
 if code or size!=expected:raise RuntimeError(f'{filename}: backup failed or short read {size}/{expected}; '+err.decode(errors='replace')[:200])
 os.replace(partial,target);manifest['backups'].append({'file':filename,'bytes':size,'sha256':h.hexdigest()});print(filename,size,h.hexdigest())
 (a.output/'BACKUP_MANIFEST.json').write_text(json.dumps(manifest,indent=2)+'\n',encoding='utf-8')
try:
 disk_sectors=int(command('cat /sys/class/block/mmcblk0/size').strip())
 stream('dd if=/dev/mmcblk0 bs=512 count=8192','gpt-first-4MiB.img',4194304)
 stream('dd if=/dev/mmcblk0 bs=512 skip='+str(disk_sectors-8192)+' count=8192','gpt-last-4MiB.img',4194304)
 wanted={'misc','nvcfg','nvdata','nvram','protect1','protect2','seccfg','boot_para','raw1','raw2','qraw','fh_data','fh_conf','boot_a','rootfs_a','boot_b','rootfs_b','connsys_gnss_a','connsys_gnss_b'}
 for part in parts:
  if part['name'] in wanted:
   assert re.fullmatch(r'[A-Za-z0-9_]+',part['name'])
   stream('dd if=/dev/'+part['device']+' bs=1048576',part['name']+'.img',part['bytes'])
 size=int(command('wc -c < /fhdata/wlan/e2p').strip());assert size==987136,'Unexpected own calibration size'
 stream('cat /fhdata/wlan/e2p','own-wlan-e2p.bin',size)
 if a.full_disk:
  disk_size=int(command('cat /sys/class/block/mmcblk0/size').strip())*512
  stream('dd if=/dev/mmcblk0 bs=1048576','mmcblk0-full.img',disk_size)
  for boot in ['mmcblk0boot0','mmcblk0boot1']:
   size=int(command('cat /sys/class/block/'+boot+'/size').strip())*512;stream('dd if=/dev/'+boot+' bs=1048576',boot+'.img',size)
 (a.output/'SHA256SUMS.txt').write_text(''.join(x['sha256']+'  '+x['file']+'\n' for x in manifest['backups']),encoding='utf-8')
 print('Read-only backup complete. Keep this private; do not publish or restore it to another CPE.')
finally:client.close()

