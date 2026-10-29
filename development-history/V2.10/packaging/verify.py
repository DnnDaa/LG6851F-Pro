from pathlib import Path
import base64,hashlib,json,random,re,shlex,subprocess,tarfile,xml.etree.ElementTree as ET,zipfile
H=Path(__file__).resolve().parent;R=H.parents[1];B=R/'build/release-v2.10';O=R/'release-assets/V2.10'
def load(p):
 with tarfile.open(p) as t:return {m.name:(m,t.extractfile(m).read() if m.isfile() else None) for m in t}
source=load(R/'build/systemfix-20261029-94-b/assembled-root.tar')
sha=lambda b:hashlib.sha256(b).hexdigest()
secrets=set()
for name in ['etc/config/wireless','etc/board.json','etc/shadow','etc/lg6851f-power/plug.json']:
 if name not in source:continue
 s=source[name][1].decode(errors='replace')
 secrets.update(x for x in re.findall(r"(?:option key |\"(?:key|root_password_plain|token)\"\s*:\s*)['\"]([^'\"]+)['\"]",s) if len(x)>=8 and x!='password')
 secrets.update(re.findall(r'\$[156y]\$[^:\s]+',s))
report={}
for kind,slot in [('private','a'),('public','a'),('public','b')]:
 path=B/f'{kind}-{slot}';tree=load(path/'assembled-root.tar')
 pseudo=path/'readback.pseudo'
 subprocess.run([str(R/'tools/msys/usr/bin/unsquashfs.exe'),'-pf',str(pseudo),str(path/'rootfs.squashfs')],stdout=subprocess.DEVNULL,stderr=subprocess.DEVNULL,check=True)
 head,data=pseudo.read_bytes().split(b'#\n# START OF DATA - DO NOT MODIFY\n#\n',1)
 rows={f[0]:f for line in head.decode().splitlines() if (f:=shlex.split(line))}
 for n,(m,b) in tree.items():
  f=rows[n];assert int(f[3],8)==m.mode and int(f[4])==m.uid and int(f[5])==m.gid,n
  if b is not None:assert data[int(f[7]):int(f[7])+int(f[6])]==b,n
  elif m.issym():assert f[1]=='S' and f[6]==m.linkname,n
 modules=[n for n in tree if n.endswith('.ko')];assert len(modules)==120
 assert all(tree[n][1]==source[n][1] for n in modules)
 for n in ['lib/netifd/wireless/vendor618.sh','lib/netifd/vendor618-client64.sh','usr/sbin/lg6851f-hostapd','www/luci-static/resources/view/fiberhome/home_stable_v20261026_71.js','etc/uci-defaults/96-lg6851f-lan-ipv6-server','etc/uci-defaults/95-lg6851f-wan6-dynamic']:
  if n in source:assert tree[n][1]==source[n][1],n
 # Every registered file hash is consistent with its retained/edited body.
 folder='';current=None;apk=0;sha256_160=False
 for line in tree['lib/apk/db/installed'][1].decode().splitlines():
  if not line:sha256_160=False;folder='';current=None
  elif line.startswith('f:'):sha256_160='S' in line[2:]
  elif line.startswith('F:'):folder=line[2:];current=None
  elif line.startswith('R:'):current=(folder+'/'+line[2:]).lstrip('/')
  elif line.startswith('Z:') and current in tree and tree[current][1] is not None:
   body=tree[current][1];digest=hashlib.sha256(body).digest()[:20] if sha256_160 else hashlib.sha1(body).digest()
   assert line[2:]=='Q1'+base64.b64encode(digest).decode(),current;apk+=1
 s=tree['lib/upgrade/platform.sh'][1].decode();healthy=tree['usr/sbin/lg6851f-a-mark-healthy'][1].decode()
 assert f'bootslot={slot}' in s and f'bootslot={slot}' in healthy
 part=26 if slot=='a' else 39;retry=2061 if slot=='a' else 2066
 assert f'root=/dev/mmcblk0p{part}' in healthy and f'seek={retry}' in healthy
 expected="0f00${bootctrl_before:4:6}0e${bootctrl_before:12:8}" if slot=='a' else "0e${bootctrl_before:2:8}0f00${bootctrl_before:14:6}"
 assert expected in s
 # Exercise actual shell slice expressions against randomized boot-control records.
 expr=re.search(r'expected_bootctrl="([^\n]+)"',s).group(1)
 for _ in range(10000):
  old=random.randbytes(10);hexold=old.hex()
  actual=re.sub(r'\$\{bootctrl_before:(\d+):(\d+)\}',lambda m:hexold[int(m[1]):int(m[1])+int(m[2])],expr)
  x=bytearray(old);x[0]=15 if slot=='a' else 14;x[5]=14 if slot=='a' else 15;x[1 if slot=='a' else 6]=0
  assert bytes.fromhex(actual)==x
 if kind=='public':
  for n,(m,b) in tree.items():
   if b is not None:
    assert not any(secret.encode() in b for secret in secrets),('private secret retained',n)
  assert 'etc/lg6851f-vendor618/firmware/lg6851f-board-e2p' not in tree
  assert not any(n.startswith('etc/dropbear/') and b is not None for n,(m,b) in tree.items())
  assert not any(n.startswith('root/') and b is not None for n,(m,b) in tree.items())
  assert not json.loads(tree['etc/board.json'][1]).get('credentials')
  assert b'PUBLIC-FIRSTBOOT-REPLACES-THIS' in tree['etc/config/wireless'][1]
  assert b'-o ro,noload /dev/mmcblk0p44' in tree['usr/sbin/lg6851f-public-calibration'][1]
  assert tree['usr/sbin/lg6851f-vendor618-start'][1].count(b'/usr/sbin/lg6851f-public-calibration')==1
 out=O if kind=='public' else R/'release-assets/20261029-systemfix-94-a'
 manifest=json.loads((out/f'firmware-manifest-{slot.upper()}.json').read_bytes())
 web=next(k for k in manifest['packages'] if k.endswith('tar.gz'));mtk=next(k for k in manifest['packages'] if k.endswith('.zip'))
 with tarfile.open(out/web) as t:
  names=t.getnames();control=dict(x.split('=',1) for x in t.extractfile('CONTROL').read().decode().splitlines())
  assert names[0]=='CONTROL' and control['slot']==slot
  assert control[f'touches_slot_{"b" if slot=="a" else "a"}']=='0'
  payload={n:t.extractfile(n).read() for n in manifest['images']}
 with zipfile.ZipFile(out/mtk) as z:
  assert z.testzip() is None
  for n,b in payload.items():assert z.read('download_package/'+n)==b and sha(b)==manifest['images'][n]['sha256']
  xml=ET.fromstring(z.read('download_package/LG6851F_scatter.xml'))
  selected={p.findtext('partition_name') for p in xml.iter('partition_index') if p.findtext('is_download')=='true'}
  assert selected=={f'boot_{slot}',f'rootfs_{slot}',f'connsys_gnss_{slot}'}
  assert not any(n.endswith(f'_{"b" if slot=="a" else "a"}.img') for n in z.namelist())
 for n,v in manifest['packages'].items():assert sha((out/n).read_bytes())==v['sha256']
 report[f'{kind}-{slot}']={'passed':True,'squashfs_entries_read_back':len(tree),'unchanged_kernel_modules':len(modules),'apk_file_hashes_verified':apk,'random_bootctrl_records_tested':10000,'web_mtk_images_identical':True,'zip_crc_passed':True,'public_privacy_checked':kind=='public','physical_flash_or_boot_tested':False}
 print('VERIFIED',kind,slot,flush=True)
(H/'verification.json').write_text(json.dumps(report,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
(O/'verification.json').write_text(json.dumps(report,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
