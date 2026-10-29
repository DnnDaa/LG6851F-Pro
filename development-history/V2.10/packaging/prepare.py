from pathlib import Path
import copy,tarfile,io,json,re,hashlib,base64

H=Path(__file__).resolve().parent; R=H.parents[1]
B=R/'build/systemfix-20261029-94-b'
BUILD=R/'build/release-v2.10'; BUILD.mkdir(exist_ok=True)
OLD=Path('E:/LG6851F-Pro-GitHub-20261015/repository/files')
def js(x):return (json.dumps(x,ensure_ascii=False,indent=2)+'\n').encode()
def sha(x):return hashlib.sha256(x).hexdigest()
with tarfile.open(B/'assembled-root.tar') as t:
 source={m.name:(copy.copy(m),t.extractfile(m).read() if m.isfile() else None) for m in t}
def put(tree,n,b,mode=None):
 m=copy.copy(tree[n][0]) if n in tree else tarfile.TarInfo(n)
 m.size=len(b);m.uid=m.gid=0
 if mode is not None:m.mode=mode
 elif n not in tree:m.mode=0o755 if b.startswith(b'#!') else 0o644
 tree[n]=(m,b)
def replace(tree,n,old,new):
 b=tree[n][1];assert old in b,(n,old);put(tree,n,b.replace(old,new))

def a_slot(tree):
 for n in ['usr/sbin/lg6851f-vendor618-start','etc/uci-defaults/99-lg6851f-private-board-56','etc/uci-defaults/99-lg6851f-core-modes-57']:
  replace(tree,n,b'bootslot=b',b'bootslot=a')
  if b'root=/dev/mmcblk0p39' in tree[n][1]:replace(tree,n,b'root=/dev/mmcblk0p39',b'root=/dev/mmcblk0p26')
 n='usr/sbin/lg6851f-a-mark-healthy';s=tree[n][1].decode()
 for a,b in [('bootslot=b','bootslot=a'),('mmcblk0p39','mmcblk0p26'),('B-only','A-only'),('every A byte','every B byte'),('lg6851f-b-','lg6851f-a-'),('B retry','A retry'),('B LAN','A LAN'),('B_BOOT','A_BOOT'),('skip=2066','skip=2061'),('seek=2066','seek=2061')]:s=s.replace(a,b)
 s=s.replace('cut -c1-12','cut -c1-2').replace('cut -c15-20','cut -c5-20')
 put(tree,n,s.encode())
 n='lib/upgrade/platform.sh';s=tree[n][1].decode()
 for a,b in [('connsys_gnss_b','connsys_gnss_a'),('rootfs_b','rootfs_a'),('boot_b','boot_a'),('mt6990-b-signed','mt6990-a-signed'),('bootslot=b','bootslot=a'),('touches_slot_a','touches_slot_b'),('slot_a=untouched','slot_b=untouched'),('mmcblk0p38','mmcblk0p25'),('mmcblk0p39','mmcblk0p26'),('mmcblk0p27','mmcblk0p14'),('= 38 ]','= 25 ]'),('= 39 ]','= 26 ]'),('= 27 ]','= 14 ]'),('= b ]','= a ]'),('B-slot','A-slot'),('B-only','A-only'),('slot B','slot A'),('baseline b','baseline a'),('priority-and-b-retry','priority-and-a-retry'),('B-attempt','A-attempt'),('Slot A is untouched; B','Slot B is untouched; A')]:s=s.replace(a,b)
 s=s.replace('expected_bootctrl="0e${bootctrl_before:2:8}0f00${bootctrl_before:14:6}"','expected_bootctrl="0f00${bootctrl_before:4:6}0e${bootctrl_before:12:8}"')
 s=s.replace("printf '\\016' | dd of=/dev/mmcblk0p1 bs=1 seek=2060", "printf '\\017' | dd of=/dev/mmcblk0p1 bs=1 seek=2060")
 s=s.replace("printf '\\017' | dd of=/dev/mmcblk0p1 bs=1 seek=2065", "printf '\\016' | dd of=/dev/mmcblk0p1 bs=1 seek=2065")
 s=s.replace('seek=2066','seek=2061').replace('priority A=0e B=0f B_retry=00 success/up_type/A_state=preserved','priority A=0f B=0e A_retry=00 success/up_type/B_state=preserved')
 start=s.index('\t# Prepare a fresh A-slot boot attempt.')
 end=s.index('\tdd if=/dev/mmcblk0p1',start)
 s=s[:start]+'\t# Select A priority 0f, B priority 0e; reset only A retry.\n\t# Preserve both success/up_type fields and all other B state.\n'+s[end:]
 put(tree,n,s.encode())
 for n in ['usr/sbin/lg6851f-vendor618-start']:
  put(tree,n,tree[n][1].replace(b'B-slot OpenWrt root',b'A-slot OpenWrt root'))

def public(tree):
 removed=[]
 for n in list(tree):
  if n.startswith(('root/','etc/dropbear/','etc/ssh/','etc/lg6851f-cellular/','etc/lg6851f-power/','etc/lg6851f-wireless/')) or n in ['etc/shadow-','etc/lg6851f-vendor618/firmware/lg6851f-board-e2p','etc/lg6851f-vendor618/hostapd24.conf','etc/lg6851f-vendor618/hostapd5.conf','etc/ethers']:
   if tree[n][1] is not None:removed.append(n);del tree[n]
 for n in ['etc/config/nikki','etc/config/openclash']:
  # Take the package's unconfigured defaults, not historical subscriptions.
  if (OLD/n).is_file():put(tree,n,(OLD/n).read_bytes().replace(b'\r\n',b'\n'))
 for n in list(tree):
  if n.startswith('etc/openclash/custom/') or n.startswith('etc/openclash/overwrite/') or n=='etc/nikki/mixin.yaml':
   p=OLD/n
   if p.is_file():put(tree,n,p.read_bytes().replace(b'\r\n',b'\n'))
   elif tree[n][1] is not None:removed.append(n);del tree[n]
 put(tree,'etc/shadow',b'root::0:0:99999:7:::\ndaemon:*:0:0:99999:7:::\nftp:*:0:0:99999:7:::\nnetwork:*:0:0:99999:7:::\nnobody:*:0:0:99999:7:::\n',0o600)
 board=json.loads(tree['etc/board.json'][1]);board.pop('credentials',None);board.pop('wlan',None);board.pop('network_device',None)
 board['network']['modem5g'].pop('plmn',None);board['network']['modem5g']['apn']=''
 put(tree,'etc/board.json',js(board),0o600)
 put(tree,'etc/board.d/06-lg6851f-recovery-wifi',b'''#!/bin/sh
. /lib/functions/uci-defaults.sh
board_config_update
ucidef_set_country CN
key="$(dd if=/dev/urandom bs=16 count=1 2>/dev/null | hexdump -v -e '1/1 "%02x"')"
[ "${#key}" = 32 ] || exit 1
ucidef_set_wireless all LG6851F-Pro psk2 "$key"
board_config_flush
exit 0
''',0o755)
 s=tree['etc/config/network'][1].decode();s=re.sub(r"^\s*option (dhcp_default_duid|ula_prefix|plmn) .*\n",'',s,flags=re.M);s=re.sub(r"option apn '[^']*'","option apn ''",s)
 put(tree,'etc/config/network',s.encode())
 s=tree['etc/config/dhcp'][1].decode();s=re.sub(r"\nconfig host [\s\S]*$",'\n',s);put(tree,'etc/config/dhcp',s.encode())
 put(tree,'etc/config/lg6851f_modem_lock',b"config profile 'main'\n\toption autoapply '0'\n\toption mode 'none'\n")
 # Original 94 radio features and power settings; no donor SSIDs or keys.
 s=tree['etc/config/wireless'][1].decode();s=re.sub(r"option ssid '[^']*'",lambda m:"option ssid 'LG6851F-PUBLIC-Pro-"+('2G' if m.start()<s.index("config wifi-device 'radio1'") else '5G')+"'",s)
 s=re.sub(r"option key '[^']*'","option key 'PUBLIC-FIRSTBOOT-REPLACES-THIS'",s)
 put(tree,'etc/config/wireless',s.encode())
 put(tree,'etc/uci-defaults/zzzz-lg6851f-public-firstboot',(OLD/'etc/uci-defaults/zzzz-lg6851f-public-firstboot').read_bytes().replace(b'\r\n',b'\n'),0o755)
 helper=(OLD/'usr/sbin/lg6851f-public-calibration').read_bytes().replace(b'\r\n',b'\n')
 # Reading the per-unit blob only; immutable SDK EEPROM template is retained.
 put(tree,'usr/sbin/lg6851f-public-calibration',helper,0o755)
 replace(tree,'usr/sbin/lg6851f-vendor618-start',b'echo "$fw" > /sys/module/firmware_class/parameters/path',b'echo "$fw" > /sys/module/firmware_class/parameters/path\n/usr/sbin/lg6851f-public-calibration')
 for n,(m,b) in list(tree.items()):
  if b is None:continue
  if len(b)==7680 and (n.endswith('EEPROM.bin') or '/e2p' in n or 'eeprom_mt6990.bin' in n):
   # These 12 bytes are the donor MACs, not RF calibration values.
   put(tree,n,b[:4]+bytes(12)+b[16:])
  elif n.startswith('etc/lg6851f-vendor618/wireless/') and b'\0' not in b:
   s=b.decode(errors='strict');s=re.sub(r'^MacAddress\d*=.*$', 'MacAddress=',s,flags=re.M)
   s=re.sub(r'^(WPAPSK\d*|ApCliWPAPSK\d*|RADIUS_Key\d*|SAEPassword\d*|ApCliSsid\d*|ApCliBssid\d*)=.*$',lambda m:m.group(1)+'=',s,flags=re.M)
   s=re.sub(r'^SSID\d*=.*$',lambda m:m.group(0).split('=')[0]+'=LG6851F',s,flags=re.M)
   put(tree,n,s.encode())
 return removed

def apk_rehash(tree):
 n='lib/apk/db/installed';lines=tree[n][1].decode().splitlines(keepends=True);folder='';current=None;updated=[];sha256_160=False
 for i,line in enumerate(lines):
  if not line.strip():sha256_160=False;folder='';current=None
  elif line.startswith('f:'):sha256_160='S' in line[2:]
  elif line.startswith('F:'):folder=line[2:].strip();current=None
  elif line.startswith('R:'):current=(folder+'/'+line[2:].strip()).lstrip('/')
  elif line.startswith('Z:') and current in tree and tree[current][1] is not None:
   body=tree[current][1]
   digest=hashlib.sha256(body).digest()[:20] if sha256_160 else hashlib.sha1(body).digest()
   value='Z:Q1'+base64.b64encode(digest).decode()+'\n'
   if value!=line:lines[i]=value;updated.append(current)
 put(tree,n,''.join(lines).encode());return updated

report={}
for kind,slot in [('private','a'),('public','a'),('public','b')]:
 tree=dict(source)
 if slot=='a':a_slot(tree)
 removed=public(tree) if kind=='public' else []
 info=json.loads(tree['etc/lg6851f-build.json'][1]);info.update(version=('V2.10' if kind=='public' else 'current-selfuse-20261029-systemfix-94-a'),source_systemfix=94,slot=slot,target_slot=slot,public_release=kind=='public')
 put(tree,'etc/lg6851f-build.json',js(info))
 updated=apk_rehash(tree)
 path=BUILD/f'{kind}-{slot}';path.mkdir(exist_ok=True)
 with tarfile.open(path/'assembled-root.tar','w',format=tarfile.GNU_FORMAT) as t:
  for n,(m,b) in tree.items():t.addfile(m,io.BytesIO(b) if b is not None else None)
 changed=[n for n in tree if n not in source or tree[n][1]!=source[n][1]]
 report[f'{kind}-{slot}']={'changed':changed,'removed':removed,'apk_rehashed':updated,'files':len(tree),'module_hashes':{n:sha(b) for n,(m,b) in tree.items() if b is not None and n.endswith('.ko')}}
(H/'preparation-checks.json').write_bytes(js(report))
print(json.dumps({k:{'files':v['files'],'changed':len(v['changed']),'removed':len(v['removed'])} for k,v in report.items()}))
