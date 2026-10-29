// SPDX-License-Identifier: GPL-2.0-only
'use strict';
import {readfile,writefile,open,rename,unlink,lsdir,access} from 'fs';
import {connect} from 'ubus';
import * as nl80211 from 'nl80211';
const SELF='/usr/share/rpcd/ucode/lg6851f.wireless';
const CACHE='/tmp/lg6851f-wireless.snapshot.json';
const WATCH='/tmp/lg6851f-wireless.watch';
const OWNER='/tmp/lg6851f-wireless.owner';
const LOCK='/var/lock/lg6851f-radio-control.lock';
const PERSIST='/overlay/MT6990_DIAGNOSTICS/wireless-last-good.json';
const SEED='/etc/lg6851f-wireless/last-good.json';
const SUPERVISOR='/usr/sbin/lg6851f-wireless-sampler';
const INTERVAL=10, MAX_AGE=30, WATCH_TTL=45;
function readstr(path){try {let v=readfile(path);return v==null?null:trim(v);}catch(e){return null;}}
function num(v){if(v==null)return null;v=trim(''+v);return match(v,/^-?[0-9]+(\.[0-9]+)?$/)?+v:null;}
function intval(v){let n=num(v);return n==null?null:int(n);}
function monotonic(){return num(split(readstr('/proc/uptime')??'0',' ')[0])??0;}
function boot_id(){return readstr('/proc/sys/kernel/random/boot_id');}
function own_pid(){return int(match(readstr('/proc/self/stat')??'',/^[0-9]+/)?.[0]??0);}
function read_snapshot(path){try{return json(readfile(path)??'null');}catch(e){return null;}}
function usable(cache){return type(cache?.wifi)=='object'&&cache.wifi.available!=false&&cache.wifi.clients_n!=null&&type(cache.wifi.clients)=='array';}
function load(){
 let cache=read_snapshot(CACHE);if(usable(cache))return cache;
 cache=read_snapshot(PERSIST);if(usable(cache))return cache;
 return read_snapshot(SEED);
}
function publish(value){let fd=open(CACHE+'.new','w',0600);if(!fd)die('snapshot unavailable');fd.write(sprintf('%J',value));fd.close();if(!rename(CACHE+'.new',CACHE))die('snapshot publish failed');}
function unknown(){return {up:0,ssid:null,channel:null,band:null,width:null,country:null,enc:null,hidden:0,hostapd:0,clients:[],clients_n:null,radios:[],runtime:{},available:false};}
// Read only kernel interface metadata; never query a station table here.
function current_generations(){
 let out={};for(let radio in ['radio0','radio1'])out[radio]=readstr('/tmp/lg6851f-vendor618/'+radio+'.generation');return out;
}
function gate_interfaces(cache, boot) {
 let source=cache?.wifi ?? unknown(), names={ra0:true,rai0:true}, state={}, down={};
 for(let radio in source.radios??[])if(radio.ifname)names[radio.ifname]=true;
 for(let client in source.clients??[])if(client.ifname)names[client.ifname]=true;
 for(let name,radio in source.runtime??{})
  for(let iface in radio.interfaces??[])if(iface.ifname)names[iface.ifname]=true;
 let changed={},generation=current_generations();
 for(let radio,record in source.runtime??{}){
  let now=generation[radio],old=source.radio_generations?.[radio];
  // Native collection holds the same radio-control lock as reconfiguration.
  // A snapshot collected AFTER an interrupted change is authoritative even
  // when that failed transaction never published ready. Older generations
  // are still invalidated immediately during an active change.
  if(now!=null && now!=old)
   for(let iface in record.interfaces??[])if(iface.ifname)changed[iface.ifname]=true;
 }
 let known=0,running=0,unknown_state=0,signature=[];
 for(let name in names){
  if(!match(name,/^[A-Za-z0-9_.:-]{1,15}$/))continue;
  let flags=readstr('/sys/class/net/'+name+'/flags');
  if(!match(flags??'',/^0x[0-9a-fA-F]+$/)){
   // A deleted netdev is confirmed absent, not an unreadable running AP.
   // Keep unknown only when the interface still exists but flags cannot be read.
   if(!access('/sys/class/net/'+name)){state[name]=false;known++;down[name]=true;push(signature,name+'=absent');}
   else unknown_state++;
   continue;
  }
  state[name]=(int(flags,16)&1)!=0;
  known++;if(state[name])running++;else down[name]=true;
  push(signature,name+'='+(state[name]?'1':'0'));
 }
 let rebooted=cache?.boot_id!=null && cache.boot_id!=boot;
 let stopped=known>0 && !running && !unknown_state;
 if(!rebooted && !length(down) && !length(changed) && !source.confirmed_off)return cache;
 let wifi={...source},runtime={},details=[];
 wifi.clients=filter(source.clients??[],(client)=>!rebooted&&!stopped&&!down[client.ifname]&&!changed[client.ifname]);
 wifi.clients_n=length(wifi.clients);
 wifi.clients_partial=0;
 wifi.confirmed_off=stopped;
 wifi.boot_id=boot;
 // Preserve configured SSIDs, channels and hardware descriptions. Erase
 // operational rates/power and connection membership only on stopped APs.
 for(let radio in source.radios??[]){
  let detail={...radio};
  if(stopped||down[detail.ifname]||changed[detail.ifname]||rebooted){
   delete detail.rate_kbps;delete detail.txpower;
   delete detail.operating_width_mhz;delete detail.center_freq1;delete detail.center_freq2;
   delete detail.frequency;delete detail.split_80p80;
  }
  push(details,detail);
 }
 wifi.radios=details;
 for(let name,radio in source.runtime??{}){
  let current={...radio},interfaces=[],all_down=true,has_iface=false;
  for(let iface in radio.interfaces??[]){
   let item={...iface};has_iface=true;
   if(!down[item.ifname])all_down=false;
   if(stopped||down[item.ifname]||rebooted){
    item.up=false;item.iwinfo={...(item.iwinfo??{})};
    delete item.iwinfo.bitrate;delete item.iwinfo.txpower;
   }
   if(changed[item.ifname]){item.iwinfo={...(item.iwinfo??{})};delete item.iwinfo.bitrate;}
   push(interfaces,item);
  }
  current.interfaces=interfaces;
  if(stopped||rebooted||(has_iface&&all_down))current.up=false;
  runtime[name]=current;
 }
 wifi.runtime=runtime;
 if(stopped||rebooted){wifi.up=0;wifi.hostapd=0;}
 if(stopped||usable(cache)){wifi.available=true;}
 return {...(cache??{}),wifi:wifi,
  sample_id:(cache?.sample_id??'none')+':interfaces:'+join(',',signature)+':generations:'+sprintf('%J',generation)+(rebooted?':new-boot':'')};
}

function station_rate_kbps(value){let n=num(value);if(n==null||n<=0)return null;return n>=1000000?sprintf('%.2f Gbit/s',n/1000000.0):sprintf('%.1f Mbit/s',n/1000.0);}
function leases(){let rows=[];for(let line in split(readstr('/tmp/dhcp.leases')??'','\n')){let f=split(trim(line),/[ \t]+/);if(length(f)>=4)push(rows,{mac:f[1],ip:f[2],host:f[3]=='*'?null:f[3]});}return rows;}

function valid_mac(value){return type(value)=='string'&&match(value,/^([0-9a-f]{2}:){5}[0-9a-f]{2}$/)&&value!='00:00:00:00:00:00'&&(int(substr(value,0,2),16)&1)==0;}
function identity_aliases(){
 let dir='/tmp/lg6851f-wifi-identities',aliases={},owners={},names=[];
 try{names=lsdir(dir)??[];}catch(e){return aliases;}
 // These are event-produced metadata files, not another client table query.
 if(length(names)>4096)return aliases;
 for(let name in names){
  if(!match(name,/^([0-9a-f]{2}:){5}[0-9a-f]{2}-([0-9a-f]{2}:){5}[0-9a-f]{2}\.json$/))continue;
  let data=readstr(dir+'/'+name),entry=null;
  if(data==null||length(data)>2048)continue;
  try{entry=json(data);}catch(e){continue;}
  let pid=intval(entry?.pid),mld=entry?.mld_mac;
  if(pid==null||pid<2||!valid_mac(mld)||type(entry?.links)!='array'||length(entry.links)>17)continue;
  if(owners[pid]==null){
   let cmd=readfile('/proc/'+pid+'/cmdline');
   owners[pid]=cmd!=null&&index(cmd,'/usr/sbin/lg6851f-hostapd\x00')==0;
  }
  if(!owners[pid])continue;
  for(let mac in entry.links){
   if(!valid_mac(mac))continue;
   // Ambiguous aliases must never borrow a different device's hostname.
   if(aliases[mac]!=null&&aliases[mac]!=mld)aliases[mac]=false;
   else if(aliases[mac]!==false)aliases[mac]=mld;
  }
 }
 return aliases;
}

function pick(source,keys){let out={};for(let key in keys)if(source?.[key]!=null)out[key]=source[key];return out;}
function radio_view(wireless,wifi){
 let result={};
 for(let name,radio in wireless??{}){
  let r=pick(radio,['up','pending','autostart','disabled','retry_setup_failed']);
  r.config=pick(radio.config,['type','band','channel','country','htmode','disabled','txpower','vendor_power_boost','vendor_power_level']);r.interfaces=[];
  let band=r.config.band,label=band=='2g'?'MediaTek 2.4 GHz':band=='5g'?'MediaTek 5 GHz':'MediaTek';
  r.iwinfo={hardware:{name:label},hwmodes:band=='2g'?['n','ax','be']:['n','ac','ax','be'],hwmodes_text:band=='2g'?'be/ax/n':'be/ax/ac/n'};
  for(let iface in radio.interfaces??[]){
   let n=pick(iface,['ifname','section','disabled']);
   n.config=pick(iface.config,['device','mode','ssid','encryption','hidden','disabled','network','bssid']);
   let detail=null;for(let d in wifi.radios??[])if(d.ifname==n.ifname){detail=d;break;}
   let channel=num(detail?.channel??r.config.channel),freq=null;
   if(channel!=null)freq=band=='2g'?(channel==14?2484:2407+5*channel):(band=='5g'?5000+5*channel:null);
   n.iwinfo={ssid:detail?.ssid??n.config.ssid,mode:n.config.mode=='sta'?'Client':'Master',bssid:detail?.bssid??readstr('/sys/class/net/'+n.ifname+'/address'),channel:channel,frequency:freq,country:r.config.country};
   if(detail?.txpower!=null)n.iwinfo.txpower=detail.txpower;
   if(detail?.rate_kbps!=null)n.iwinfo.bitrate=detail.rate_kbps;
   let enc=n.config.encryption??'none';
   n.iwinfo.encryption={enabled:enc!='none',wpa:match(enc,/sae/)?[3]:(match(enc,/psk2|wpa2/)?[2]:[]),authentication:match(enc,/sae/)?['sae']:(match(enc,/psk/)?['psk']:[]),ciphers:enc=='none'?[]:['ccmp']};
   push(r.interfaces,n);
  }
  result[name]=r;
 }
 return result;
}
function width_mhz(value){let m=match(value??'',/^(HT|VHT|HE|EHT)([0-9]+)$/);return m?int(m[2]):null;}
function station_width(info){if(info?.width_320)return 320;if(info?.width_160||info?.width_80p80)return 160;if(info?.width_80)return 80;if(info?.width_40||(info!=null&&info['40_mhz_width'])||info?._40_mhz_width)return 40;if(info?.width_10)return 10;if(info?.width_5)return 5;return null;}
// Native nl80211 rate information -> the existing LuCI iwinfo rate schema.
function native_rate(info){
 if(type(info)!='object')return {};
 let out={},kbps=num(info.bitrate32??info.bitrate);
 if(kbps!=null&&kbps>0)out.rate=kbps*100;
 let mode=null;
 if(info.eht_mcs!=null)mode='eht';
 else if(info.he_mcs!=null)mode='he';
 else if(info.vht_mcs!=null)mode='vht';
 else if(info.mcs!=null)mode='ht';
 if(mode!=null){
  out[mode]=true;
  let mcs=intval(mode=='ht'?info.mcs:info[mode+'_mcs']);
  if(mcs!=null&&mcs>=0)out.mcs=mcs;
  let nss=intval(info[mode+'_nss']);
  if(nss!=null&&nss>0)out.nss=nss;
  if(mode=='he'||mode=='eht'){
   let gi=intval(info[mode+'_gi']);
   if(gi!=null&&gi>=0)out[mode+'_gi']=gi;
   let ru=intval(info[mode+'_ru_alloc']);
   if(ru!=null&&ru>=0)out[mode+'_ru_alloc']=ru;
  }
  if(mode=='he'){
   let dcm=intval(info.he_dcm);
   if(dcm!=null&&dcm>=0)out.he_dcm=dcm;
  }
 }
 if(info.short_gi==true)out.short_gi=true;
 let mhz=station_width(info);
 // A native MCS rate without any width flags uses nl80211's default 20 MHz.
 if(mhz==null&&mode!=null)mhz=20;
 if(mhz!=null)out.mhz=mhz;
 return out;
}

function rate(value,mhz,info){if(type(info)=="object"&&num(info.rate)!=null&&info.rate>0)return info;let m=match(value??'',/^([0-9.]+) ([MG])bit\/s$/);if(!m)return {};let out={rate:(+m[1])*(m[2]=='G'?1000000:1000)};if(mhz!=null)out.mhz=mhz;return out;}
export function snapshot_view(cache,stamp,boot){
 let age=cache?.boot_id==boot?stamp-(cache?.mono??-1000000):time()-(cache?.ts??0);
 let available=usable(cache),fresh=available&&cache.boot_id==boot&&age>=0&&age<=MAX_AGE;
 let wifi=available?cache.wifi:unknown(),clients=[];
 wifi.available=available;wifi.fresh=fresh;wifi.stale=available&&!fresh;
 wifi.sample_id=available?cache.sample_id:null;wifi.sample_age=available&&age>=0?age:null;
 wifi.sample_ts=cache?.ts??null;wifi.boot_id=boot;
 for(let c in wifi.clients??[])push(clients,{mac:c.mac,mld_mac:c.mld_mac,ifname:c.ifname,signal:c.signal,noise:null,connected_time:c.connected_time,ip:c.ip,host:c.host,rx:rate(c.rx,c.rx_mhz,c.rx_info),tx:rate(c.tx,c.tx_mhz,c.tx_info)});
 return {fresh:fresh,available:available,stale:available&&!fresh,age:available&&age>=0?age:null,
  sample_id:available?cache.sample_id:null,source:'wireless-management',interval:INTERVAL,
  partial:wifi.clients_n==null,radios:wifi.runtime??{},clients:clients,wifi:wifi};
}
function owner_alive(){
 let fields=split(readstr(OWNER)??'',' '),pid=intval(fields[0]),stamp=num(fields[1]),age=monotonic()-(stamp??-1000);
 if(pid==0&&age>=0&&age<3)return true;
 let cmd=pid>1?readfile(sprintf('/proc/%d/cmdline',pid)):null;
 if(cmd==null||index(cmd,SUPERVISOR)<0)return false;
 if(age>=0&&age<=25)return true;
 // Only this exact supervisor may be stopped. A held driver flock is never removed.
 system(sprintf('kill -KILL %d 2>/dev/null',pid));
 return true; // Start a replacement on the next request, after the old owner exits.
}
function kick(){
 let guard=open('/tmp/lg6851f-wireless.start.lock','a',0600);if(!guard||!guard.lock('xn')){guard?.close();return;}
 if(!owner_alive()){writefile(OWNER,'0 '+monotonic());system('PATH=/bin:/usr/bin:/sbin:/usr/sbin; export PATH; setsid nice -n 10 '+SUPERVISOR+' >/dev/null 2>&1 </dev/null &');}
 guard.lock('u');guard.close();
}
export function peek(){
 let boot=boot_id();
 let result=snapshot_view(gate_interfaces(load(),boot),monotonic(),boot);
 result.health=readstr('/tmp/lg6851f-wireless.health')??'waiting';
 if(!result.fresh)result.error=index(['timeout','unavailable','driver_busy'],result.health)>=0?result.health:null;
 return result;
}
export function status(){
 let stamp=monotonic(),previous=num(readstr(WATCH));
 writefile(WATCH,''+stamp);
 let cache=read_snapshot(CACHE),age=stamp-(cache?.mono??-1000);
 if((previous==null||stamp-previous>15)&&(cache?.boot_id!=boot_id()||age<0||age>=INTERVAL))writefile('/tmp/lg6851f-wireless.wake','1');
 kick();
 // RPC reads cached associations plus kernel interface flags only; no netifd, hostapd or driver station queries.
 return peek();
}
function collect_native(previous, leases) {
 let out={up:0,ssid:null,channel:null,band:null,width:null,country:null,enc:null,
  hidden:0,hostapd:0,clients:[],clients_n:null,radios:[]};
 let ub=null,wireless=null;
 try {ub=connect();wireless=ub.call('network.wireless','status');}catch(e){}
 if(type(wireless)!='object'||!length(wireless)){try{ub?.disconnect();}catch(e){}return null;}
 for(let name,radio in wireless)if(radio.pending){try{ub?.disconnect();}catch(e){}return null;}
 if(type(wireless)!='object'){try{ub?.disconnect();}catch(e){}return out;}
 let by_mac={},seen={},queried={},errors=0,ok=0,primary=null,aliases=identity_aliases();
 let remembered={};
 for(let key,value in previous?.identity_names??{})if(valid_mac(key)&&type(value)=='string'&&length(value)>0&&length(value)<=253)remembered[key]=value;
 // Names are identity hints only. Current association membership still comes
 // exclusively from the native dump. Preserve a name when DHCP renews with '*'.
 for(let l in leases ?? []){
  let mac=lc(l.mac);by_mac[mac]=l;
  if(valid_mac(mac)&&l.host)remembered[mac]=l.host;
 }
 if(length(remembered)>4096)remembered={};
 out.identity_names=remembered;
 for(let name in wireless) {
  let radio=wireless[name];
  if(type(radio)!='object' || radio.disabled || !radio.up)continue;
  let cfg=radio.config ?? {};
  for(let iface in radio.interfaces ?? []) {
   let icfg=iface.config ?? {},dev=iface.ifname;
   if(!dev || queried[dev] || iface.disabled || icfg.disabled==true || icfg.disabled==1 || icfg.disabled=='1' || index(['ap','ap-wds'],icfg.mode ?? 'ap')<0)continue;
   queried[dev]=1;
   let band=({'2g':'2.4 GHz','5g':'5 GHz','6g':'6 GHz'})[cfg.band] ?? cfg.band;
   let detail={ifname:dev,ssid:icfg.ssid,channel:cfg.channel,band:band,
    width:cfg.htmode,country:cfg.country,enc:icfg.encryption,hidden:icfg.hidden==true || icfg.hidden==1 || icfg.hidden=='1'?1:0};
   // Display the configured effective tier from this same netifd snapshot.
   // No additional driver/table query; an unchecked gain switch is Power0.
   let tier=''+(cfg.vendor_power_level??'1');
   let off=cfg.vendor_power_boost===false || cfg.vendor_power_boost===0 || cfg.vendor_power_boost==='0';
   detail.power_level=off||!match(tier,/^[123]$/)?0:int(tier);
   // Reuse the existing slow Wi-Fi snapshot, using direct GET_INTERFACE.
   // No shell/iwinfo child and no synchronous call back into rpcd.
   detail.txpower=null;
   try {
    let native=nl80211.request(nl80211.const.NL80211_CMD_GET_INTERFACE,0,{dev:dev});
    if(type(native)=='object') {
     detail.bssid=native.mac ?? readstr('/sys/class/net/'+dev+'/address');
     let mbm=num(native.wiphy_tx_power_level),freq=num(native.wiphy_freq);
     // Measured operating width, independent of configured maximum / auth HT20.
     let widths={'0':20,'1':20,'2':40,'3':80,'4':160,'5':160,'6':5,'7':10,'8':1,'9':2,'10':4,'11':8,'12':16,'13':320};
     detail.operating_width_mhz=widths[''+native.channel_width]??null;
     detail.split_80p80=native.channel_width==4;
     detail.frequency=freq;detail.center_freq1=num(native.center_freq1);detail.center_freq2=num(native.center_freq2);
     if(mbm!=null && mbm>=0 && mbm<=6000)detail.txpower=mbm/100.0;
     if(type(native.ssid)=='string' && length(native.ssid))detail.ssid=native.ssid;
     if(freq==2484)detail.channel=14;
     else if(freq>=2412 && freq<=2472)detail.channel=int((freq-2407)/5);
     else if(freq>=5000 && freq<5925)detail.channel=int((freq-5000)/5);
    }
   }catch(e){}
   push(out.radios,detail);out.up=1;out.hostapd=1;
   if(!primary || cfg.band=='5g')primary=detail;
   let stations=null,source='nl80211';
   try {
    /* RPC plugins must never synchronously call another method of rpcd.
     * Cold-start status executes live_once inside rpcd: calling its iwinfo
     * object here deadlocks session/auth and therefore the whole web UI.
     * Query the same driver GET_STATION dump directly over netlink instead. */
    let answer=nl80211.request(nl80211.const.NL80211_CMD_GET_STATION,nl80211.const.NLM_F_DUMP,{dev:dev});
    if(type(answer)=='array') {
     stations=[];
     for(let station in answer) {
      let info=station.sta_info ?? {};
      push(stations,{mac:station.mac,signal:info.signal ?? info.signal_avg,
       connected_time:info.connected_time,
       tx:native_rate(info.tx_bitrate),
       rx:native_rate(info.rx_bitrate),
       rx_mhz:station_width(info.rx_bitrate),tx_mhz:station_width(info.tx_bitrate)});
     }
    }
   }catch(e){}
   /* The integrated stock driver implements native GET_STATION dumps. Do
    * not restrict collection to radio0 or assume hostapd owns association
    * data. Hostapd remains a fallback when native station queries fail. */
   if(stations==null) {
    source='hostapd';
    try {let answer=ub.call('hostapd.'+dev,'get_clients');
     if(type(answer?.clients)=='object') {
      stations=[];out.hostapd=1;
      for(let mac in answer.clients)push(stations,{...answer.clients[mac],mac:mac});
     }
    }catch(e){}
   }
   if(stations==null){errors++;continue;}
   ok++;let best=0;
   for(let c in stations) {
    let mac=lc(c.mac ?? '');
    if(!match(mac,/^([0-9a-f]{2}:){5}[0-9a-f]{2}$/) || c.associated==false || c.authorized==false || c.authenticated==false)continue;
    let mld=aliases[mac]||null,lease=by_mac[mac]??(mld?by_mac[mld]:null),signal=intval(c.signal);
    let txk=source=='nl80211'?intval(c.tx?.rate):null,rxk=source=='nl80211'?intval(c.rx?.rate):null;
    if(txk!=null && txk>best)best=txk;
    if(rxk!=null && rxk>best)best=rxk;
    let item={mac:mac,mld_mac:mld,ip:lease?.ip,host:lease?.host??remembered[mld??mac],
     signal:signal!=null && signal<0 && signal>=-127?signal:null,
     tx:source=='nl80211'?station_rate_kbps(c.tx?.rate):null,
     rx:source=='nl80211'?station_rate_kbps(c.rx?.rate):null,
     rx_info:source=='nl80211'?c.rx:{},
     tx_info:source=='nl80211'?c.tx:{},
     rx_mhz:source=='nl80211'?c.rx_mhz:null,
     tx_mhz:source=='nl80211'?c.tx_mhz:null,
     ifname:dev,band:band,source:source,connected_time:intval(c.connected_time)};
    /* Membership comes from the current association table, never DHCP.
     * MAC normalization avoids double counting in repeated radio entries. */
    if(seen[mac]!=null) {
     let old=out.clients[seen[mac]];
     if(item.signal!=null && (old.signal==null || item.signal>old.signal))out.clients[seen[mac]]=item;
    }else{seen[mac]=length(out.clients);push(out.clients,item);}
   }
   detail.rate_kbps=best||null;
  }
 }
 out.runtime=radio_view(wireless,out);
 if(primary)for(let key in ['ssid','channel','band','width','country','enc','hidden'])out[key]=primary[key];
 out.clients_n=errors ? null : length(out.clients);
 out.clients_partial=errors && ok?1:0;
 out.assoc_sources=ok;
 try{ub?.disconnect();}catch(e){}
 return out;
}

export function collect_once(){
 let boot=boot_id(),previous=gate_interfaces(load(),boot);
 // A confirmed interface stop is a valid empty association table, not a
 // sampling failure that should retain old station membership forever.
 if(previous?.wifi?.confirmed_off)return previous.wifi;
 let wifi=collect_native(previous?.wifi,leases());
 if(!wifi)return null;
 wifi.radio_generations=current_generations();
 wifi.available=type(wifi.runtime)=='object';
 return gate_interfaces({boot_id:boot,wifi:wifi},boot)?.wifi;
}
export function collect_publish(){
 // Separate from the supervisor lock. If the kernel leaves a task uninterruptible,
 // its file description retains this lock, preventing any second driver reader.
 let guard=open(LOCK,'a',0600);if(!guard||!guard.lock('xn')){guard?.close();return 75;}
 let wifi=null;try{wifi=collect_once();}catch(e){}
 if(!wifi||!wifi.available||wifi.clients_n==null){guard.lock('u');guard.close();return 1;}
 let value={boot_id:boot_id(),mono:monotonic(),ts:time(),sample_id:boot_id()+':'+own_pid()+':'+monotonic(),wifi:wifi};
 publish(value);
 // Persist topology/identity changes only, at most every fifteen minutes.
 // Rates, RSSI and observation times stay in RAM; the UI keeps its reboot seed.
 let saved=read_snapshot(PERSIST);
 function persistence_key(item){
  let radios=[],clients=[];
  for(let r in item?.wifi?.radios??[])push(radios,join('|',[r.ifname,r.ssid,r.channel,r.width,r.enc,r.hidden,r.power_level]));
  for(let c in item?.wifi?.clients??[])push(clients,join('|',[c.ifname,c.mac,c.mld_mac,c.ip,c.host]));
  sort(radios);sort(clients);
  return sprintf('%J',[item?.wifi?.confirmed_off??false,radios,clients]);
 }
 if(!saved||saved.boot_id!=value.boot_id||
    (value.mono-(saved.mono??-1000)>=900 && persistence_key(saved)!=persistence_key(value))){
  let fd=open(PERSIST+'.new','w',0600);
  if(fd){fd.write(sprintf('%J',value));fd.close();rename(PERSIST+'.new',PERSIST);}
 }
 guard.lock('u');guard.close();return 0;
}
