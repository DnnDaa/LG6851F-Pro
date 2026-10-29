// One preflight snapshot, before any native BSS is removed or restarted.
'use strict';
import {readfile} from 'fs';
import {cursor} from 'uci';
import {bss_plan,mlo_group} from '/usr/share/ucode/lg6851f/mlo-policy.uc';

function truth(v){return v==true||v=='1';}
function scalar(v){return v==null?'':v==true?'1':v==false?'0':''+v;}
function networks(v){let a=type(v)=='array'?[...v]:split(v??'',/\s+/);a=filter(a,(x)=>length(x)>0);sort(a);return join(' ',a);}
function prepare(u,radio,data){
 if(radio!='radio0'&&radio!='radio1')die('Invalid radio identity');
 if(type(data?.interfaces)=='array'&&!length(data.interfaces))data.interfaces={};
 if(type(data?.config)!='object'||type(data?.interfaces)!='object')die('Invalid netifd snapshot');
 let dev=u.get_all('wireless',radio);
 for(let key in ['band','htmode','country','channel'])
  if(dev?.[key]!=null&&data.config[key]!=null&&scalar(dev[key])!=scalar(data.config[key]))die('Radio configuration changed during apply');
 let plan=bss_plan(u,radio),out=[],used={};
 for(let id,entry in data.interfaces){let c=entry?.config;if(c?.mode=='sta'&&!truth(c.disabled)&&!truth(entry.disabled))used[id]=true;}
 // Check every runtime mapping and its credentials against the same UCI view.
 // Do not mix a newly committed UCI section with an older handler snapshot.
 for(let b in plan){
  let runtime=null,entry=null;
  for(let id,v in data.interfaces){
   if(id==b.section || v.name==b.section || v.section==b.section){
    if(runtime!=null)die('Duplicate runtime BSS identity');runtime=id;entry=v;
   }
  }
  if(runtime==null || used[runtime] || !match(runtime,/^[A-Za-z0-9_]+$/))die('Missing runtime BSS mapping');
  used[runtime]=true;
  let conf=entry?.config;if(type(conf)!='object')die('Missing runtime BSS configuration');
  for(let key in ['ssid','key','encryption','mode'])
   if(scalar(conf[key])!=scalar(b.config[key]))die('Configuration changed during wireless apply');
  if(networks(conf.network)!='lan' || networks(b.config.network)!='lan')die('Only LAN APs are supported');
  if(conf.mode!='ap' || truth(conf.disabled)||truth(entry.disabled))die('Invalid active BSS');
  if(truth(conf.vendor_mlo??conf.mlo)!=truth(b.config.vendor_mlo??b.config.mlo))die('MLO configuration changed during apply');
  if(type(conf.ssid)!='string'||length(conf.ssid)<1||length(conf.ssid)>32||match(conf.ssid,/[\r\n]/)||match(conf.key??'',/[\r\n]/))die('Invalid SSID or key line ending');
  push(out,{...b,runtime:runtime});
 }
 // Reject unrepresented active handler entries rather than silently ignoring one.
 for(let id,entry in data.interfaces){
  let conf=entry?.config;
  if(type(conf)=='object'&&!truth(conf.disabled)&&!truth(entry.disabled)&&!used[id])die('Unexpected runtime BSS');
 }
 for(let b in out){
  let group=mlo_group(u,radio,b.section);
  b.group=group.group;
  let dev=u.get_all('wireless',b.config.device),p=b.config.vendor_protocol;
  if(p!='be'&&p!='ax'&&p!='ac'&&p!='n')
   p=match(dev?.htmode??'',/^EHT/)?'be':match(dev?.htmode??'',/^HE/)?'ax':match(dev?.htmode??'',/^VHT/)?'ac':'n';
  b.protocol=p;
 }
 return out;
}
let radio=ARGV[0],data=json(readfile('/dev/stdin')??'null');
for(let b in prepare(cursor(),radio,data))
 printf('%d %s %s %d %s %s\n',b.index,b.section,b.ifname,b.group,b.runtime,b.protocol);
