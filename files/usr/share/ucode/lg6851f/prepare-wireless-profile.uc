// Prepare private runtime copies only; factory calibration and profiles stay intact.
'use strict';
import {readfile,open,rename} from 'fs';
import {cursor} from 'uci';
import {mlo_group,bss_plan,MAX_BSS} from '/usr/share/ucode/lg6851f/mlo-policy.uc';
let u=cursor(),root='/tmp/lg6851f-vendor618/wireless/mediatek/';
function rewrite(path,values,remove){
 let text=readfile(path);if(text==null)die('Missing driver profile: '+path);
 let rows=[],seen={};
 for(let line in split(text,'\n')){
  let m=match(line,/^([A-Za-z0-9_]+)=/),key=m?.[1];
  if(key&&remove?.[key])continue;
  if(key&&values[key]!=null){if(!seen[key])push(rows,key+'='+values[key]);seen[key]=true;}
  else if(length(line))push(rows,line);
 }
 for(let key,value in values)if(!seen[key])push(rows,key+'='+value);
 let fd=open(path+'.new','w',0600);if(!fd)die('Cannot prepare wireless profile');
 fd.write(join('\n',rows)+'\n');fd.close();if(!rename(path+'.new',path))die('Cannot publish wireless profile');
}
for(let radio in ['radio0','radio1']){
 let cfg=u.get_all('wireless',radio),band=radio=='radio0'?'2g':'5g';
 if(!cfg||cfg.type!='vendor618'||cfg.band!=band)die('Unsupported radio identity');
 let mode=cfg.htmode??(band=='2g'?'EHT40':'EHT160');
 let m=match(mode,/^(EHT|HE|VHT|HT)(20|40|80|160)([+-])?$/);
 if(!m||band=='2g'&&int(m[2])>40)die('Unsupported driver width');
 let width=int(m[2]),country=uc(cfg.country??'CN');
 if(!match(country,/^[A-Z][A-Z]$/))die('Invalid driver country');
 let channel=cfg.channel;
 if(channel==null||channel=='auto'||channel=='0')channel=band=='2g'?6:36;
 if(!match(''+channel,/^[0-9]+$/))die('Invalid driver channel');
 channel=int(channel);
 if(channel<1||channel>(band=='2g'?14:177))die('Invalid driver channel');
 let phy=band=='2g'?({EHT:22,HE:16,HT:9})[m[1]]:({EHT:23,HE:17,VHT:14,HT:8})[m[1]];
 if(phy==null)die('Unsupported radio mode');
 let dfs=band=='5g'&&!((channel>=36&&channel<=48&&width<=80)||(channel>=149&&width<=80));
 let ext=m[3]=='-'?0:m[3]=='+'?1:band=='2g'?(channel<=7?1:0):int(channel/4)%2;
 let values={WirelessMode:phy,Channel:channel,CountryCode:country,
  AutoChannelSelect:0,HT_BW:width==20?0:1,HT_EXTCHA:ext,
  VHT_BW:width<=40?0:width==80?1:2,EHT_ApBw:({'20':0,'40':1,'80':2,'160':3})[''+width],
  DfsEnable:dfs?1:0,IEEE80211H:dfs?1:0,
  DfsZeroWait:0,DfsDedicatedZeroWait:0,DfsZeroWaitDefault:0};
 let path=root+'lg6851f'+(band=='2g'?'24':'5')+'.dat',aps=bss_plan(u,radio),byslot={};
 for(let b in aps)byslot[''+b.index]=b;
 // Capacity is established before the original module reads the profile.
 // Additional interfaces are opened only when netifd configures that BSS.
 values.BssidNum=MAX_BSS;
 // Native old-SDK STA capacity, no profile-driven auto-connect.
 values.ApCliNum=1;values.ApCliEnable=0;
 let original={};for(let line in split(readfile(path)??'','\n')){
  let kv=match(line,/^([A-Za-z0-9_]+)=(.*)$/);if(kv)original[kv[1]]=kv[2];
 }
 // The vendor parser reads these capabilities per BSS, not per radio.
 for(let key,value in original)if(match(key,/^(HT_|VHT_|HE_|EHT_)/) && match(value,/^[0-9]+(;[0-9]+)*$/)){
  let tokens=split(''+(values[key]??value),';'),expanded=[];
  for(let i=0;i<MAX_BSS;i++)push(expanded,tokens[i]??tokens[0]);
  values[key]=join(';',expanded);
 }
 for(let key in ['WirelessMode','HT_BW','HT_EXTCHA','VHT_BW','EHT_ApBw']){
  let one=values[key]??original[key]??0;
  if(type(one)=='string')one=split(one,';')[0];
  values[key]=join(';',map([0,1,2,3,4,5,6,7],i=>one));
 }
 let groups=[],auth=[],cipher=[],hidden=[],wmm=[],isolated=[];
 for(let i=0;i<MAX_BSS;i++){
  let b=byslot[''+i],ap=b?.config,ssid=ap?.ssid??'',key=ap?.key??trim(readfile('/proc/sys/kernel/random/uuid')??''),enc=ap?.encryption??'psk2';
  // Runtime DAT files are private. Reject line injection without blocking the other radio.
  if(match(ssid,/[\r\n]/)||length(ssid)>32)ssid='lg6851f-invalid-bss';
  if(match(key,/[\r\n]/))key='';
  values['SSID'+(i+1)]=ssid;values['WPAPSK'+(i+1)]=key;
  let group=0;if(b){try{group=mlo_group(u,radio,b.section).group;}catch(e){}}
  push(groups,group);push(auth,match(enc,/^sae/)?'WPA3PSK':enc=='none'?'OPEN':'WPA2PSK');
  push(cipher,enc=='none'?'NONE':'AES');push(hidden,ap==null?1:ap.hidden??0);
  push(wmm,ap?.wmm??1);push(isolated,ap?.isolate??0);
 }
 values.MldGroup=join(';',groups);values.AuthMode=join(';',auth);values.EncrypType=join(';',cipher);
 values.HideSSID=join(';',hidden);values.WmmCapable=join(';',wmm);values.NoForwarding=join(';',isolated);
 rewrite(path,values,{});
}
