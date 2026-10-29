#!/usr/bin/env ucode
'use strict';
import {readfile} from 'fs';
// Only current netifd intent can request recovery. Never replay old conf files,
// disabled/deleted SSIDs, a stored password or a speculative interface name.
function auth_plan(status) {
 let rows=[],pending=false;
 if(type(status)!='object')die('Invalid wireless state');
 for(let name in ['radio0','radio1']) {
  let r=status[name];
  if(!r||r.config?.type!='vendor618'||r.disabled===true||r.disabled===1||r.disabled==='1'||r.autostart===false||r.autostart===0||r.autostart==='0')continue;
  let active=[];
  for(let iface in r.interfaces??[]) {
   let c=iface.config??{};
   if(iface.disabled===true||iface.disabled===1||iface.disabled==='1'||c.disabled===true||c.disabled===1||c.disabled==='1'||index(['ap','ap-wds'],c.mode??'ap')<0)continue;
   push(active,iface);
  }
  if(!length(active))continue;
  if(r.pending)pending=true;
  push(rows,'RADIO '+name);
  let missing=false;
  for(let iface in active) {
   let dev=iface.ifname;
   if(type(dev)!='string'||!match(dev,name=='radio0'?/^ra[0-7]$/:/^rai[0-7]$/)){missing=true;continue;}
   push(rows,'IFACE '+name+' '+dev);
  }
  if(missing)push(rows,'MISSING '+name);
 }
 return join('\n',['PENDING '+(pending?'1':'0'),...rows]);
}
print(auth_plan(json(readfile('/dev/stdin')))+'\n');
