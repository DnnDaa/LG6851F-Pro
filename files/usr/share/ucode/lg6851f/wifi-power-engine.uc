// SPDX-License-Identifier: GPL-2.0-only
'use strict';
import {readfile,open,rename,chmod,popen} from 'fs';
import {cursor} from 'uci';
import * as M from './wifi-power-model.uc';

const TOOL='/usr/sbin/lg6851f-mwctl';
const LOCK='/var/run/lg6851f-wifi-power.lock';
const TTL=15;
const READER_VERSION=4;

function now() {return +(split(trim(readfile('/proc/uptime')??'0'),' ')[0]);}
function quote(v) {return "'"+replace(''+v,"'","'\\''")+"'";}
function capture(args) {
 let p=popen('/usr/bin/timeout 3 '+join(' ',map(args,quote))+' 2>&1','r');
 if(!p)die('Cannot start driver command');
 let text=p.read('all')??'',code=p.close();
 if(code!=0)die('Driver command failed ('+code+'): '+substr(trim(text),0,240));
 return text;
}
function logs() {
 let p=popen('/bin/dmesg | /usr/bin/tail -n 650','r');
 if(!p)die('Cannot read firmware response');
 let text=p.read('all')??'',code=p.close();
 if(code!=0 || length(text)>1048576)die('Firmware response unavailable');
 return text;
}
function filename(radio) {return '/var/run/lg6851f-wifi-power-'+radio+'.json';}
function cached(radio) {
 let text=readfile(filename(radio));if(text==null || length(text)>32768)return null;
 try {let data=json(text);return type(data)=='object'?data:null;}catch(e){return null;}
}
function store(radio,data) {
 let path=filename(radio),fd=open(path+'.new','w',0600);
 if(!fd)die('Cannot save firmware power status');
 if(!fd.write(sprintf('%J',data))){fd.close();die('Cannot write firmware power status');}
 fd.close();if(!rename(path+'.new',path))die('Cannot publish firmware power status');chmod(path,0600);
}
function config(s) {
 let u=cursor(),cfg=u.get_all('wireless',s.radio);
 if(!cfg || cfg.type!='vendor618' || cfg.band!=(s.channel_band==0?'2g':'5g'))die('Unsupported wireless device');
 return cfg;
}
function context(s,cfg) {
 return join(':',[trim(readfile('/proc/sys/kernel/random/boot_id')??''),
  trim(readfile('/sys/class/net/'+s.iface+'/ifindex')??''),cfg.country??'',cfg.channel??'',cfg.htmode??'']);
}
function enabled(s,cfg) {
 let raw=trim(readfile('/sys/class/net/'+s.iface+'/flags')??'0');
 return cfg.disabled!='1' && ((int(raw,16)&1)!=0);
}
function query_table(s,kind,offsets) {
 let before=logs(),last='Driver response timed out';
 capture(kind=='offsets'?[TOOL,s.iface,'set','TxPowerBoostInfo=0']:[TOOL,s.iface,'set','pwr','TxPowerInfo=2']);
 for(let i=0;i<12;i++) {
  sleep(50);
  try {
   let fresh=M.appended_logs(before,logs());
   if(kind=='rates'){let power=M.parse_rates(fresh,0,s.channel_band);M.validate_snapshot(s,{offsets:offsets,power:power});return power;}
   let offsets=M.parse_offsets(fresh,0);
   for(let c in s.categories)if(length(offsets.values[c.name]??[])!=c.size)die('Incomplete driver power offset response');
   return offsets;
  }catch(e){last=''+e;}
 }
 die(last);
}
function snapshot(s) {
 let offsets=query_table(s,'offsets'),power=query_table(s,'rates',offsets);
 return M.validate_snapshot(s,{offsets:offsets,power:power});
}
function same(a,b) {return sprintf('%J',a)==sprintf('%J',b);}
function set_category(s,c,values) {
 capture([TOOL,s.iface,'set','TxPowerBoostCtrl='+c.id+':'+join(':',values)]);
}
function result(s,cfg,snap,target,ok,error,rolled_back) {
 let out=M.summary(s,snap);
 out.ok=ok;out.error=error??null;out.rollback_verified=!!rolled_back;
 out.radio=s.radio;out.iface=s.iface;out.enabled=enabled(s,cfg);out.requested_dbm=M.requested(cfg.txpower);
 out.applied_target_dbm=target;out.timestamp=now();out.context=context(s,cfg);
 out.firmware_readback=true;out.instrument_measured=false;out.reader_version=READER_VERSION;
 return out;
}
function describe_ceiling(out,p,applied) {
 out.power_policy='factory_per_rate_limit';out.configured_ceiling_dbm=p.target;
 out.factory_maximum_dbm=p.factory_peak/2.0;out.effective_ceiling_dbm=p.effective_peak/2.0;
 out.calibration_limited=p.calibration_limited;
 out.supported_maximum_dbm=int(p.factory_peak/2.0);
 out.clipped_rates=p.clipped_rates;out.preserved_rates=p.preserved_rates;
 out.applied_target_dbm=applied && p.target!=null?p.effective_peak/2.0:null;
 return out;
}
function with_lock(fn) {
 let fd=open(LOCK,'a',0600);
 if(!fd || !fd.lock('xn')){if(fd)fd.close();return {ok:false,busy:true,error:'功率控制正在应用，请稍后重试'};}
 let out;
 try{out=fn();}catch(e){out={ok:false,error:''+e};}
 fd.lock('u');fd.close();return out;
}

export function status(radio) {
 let s,cfg;
 try{s=M.spec(radio);cfg=config(s);}catch(e){return {ok:false,error:''+e};}
 if(!enabled(s,cfg))return {ok:true,radio:radio,enabled:false,requested_dbm:M.requested(cfg.txpower),reference_dbm:null,maximum_dbm:null,minimum_dbm:null};
 let old=cached(radio),ctx=context(s,cfg);
 if(old && old.reader_version==READER_VERSION && old.context==ctx && old.requested_dbm==M.requested(cfg.txpower) && now()-old.timestamp>=0 && now()-old.timestamp<TTL)return old;
 return with_lock(function() {
  let snap=snapshot(s),target=M.requested(cfg.txpower),out=result(s,cfg,snap,null,true,null,false);
  let profile=readfile('/etc/lg6851f-vendor618/wireless/mediatek/lg6851f'+(s.channel_band==0?'24':'5')+'.dat');
  try {
   if(profile==null)die('Factory wireless profile unavailable');
   let p=M.plan(s,snap,M.profile_defaults(profile,s),target);
   describe_ceiling(out,p,false);out.mcs_tables=M.mcs_tables(s,snap,p,cfg.htmode);M.verify(s,snap,p);describe_ceiling(out,p,true);
  }catch(e){
   out.ok=false;
   out.error='已保存的功率设置尚未匹配固件读回；当前参考 '+out.reference_dbm+' dBm。'+e;
  }
  // Historical apply diagnostics must not replace a fresh status measurement.
  if(old && old.context==ctx && old.ok==false && old.requested_dbm==out.requested_dbm) {
   out.previous_apply_error=old.previous_apply_error??old.error;
   out.previous_apply_rollback_verified=old.previous_apply_rollback_verified??old.rollback_verified;
  }
  store(radio,out);return out;
 });
}

export function apply(radio,value) {
 let s,target;
 try{s=M.spec(radio);target=M.requested(value);}catch(e){return {ok:false,error:''+e};}
 return with_lock(function() {
  let cfg=config(s);if(!enabled(s,cfg))die('Wireless interface is not ready');
  let before=snapshot(s),profile=readfile('/etc/lg6851f-vendor618/wireless/mediatek/lg6851f'+(s.channel_band==0?'24':'5')+'.dat');
  if(profile==null)die('Factory wireless profile unavailable');
  let defaults=M.profile_defaults(profile,s),p=M.plan(s,before,defaults,target),changed=[];
  try {
   for(let c in s.categories)if(!same(p.before[c.name],p.values[c.name])) {
    // Register before calling: a driver may mutate state before returning failure.
    push(changed,c);set_category(s,c,p.values[c.name]);
   }
   let after=length(changed)?snapshot(s):before;
   M.verify(s,after,p);
   let out=describe_ceiling(result(s,cfg,after,null,true,null,false),p,true);out.changed_categories=length(changed);out.mcs_tables=M.mcs_tables(s,after,p,cfg.htmode);
   store(radio,out);return out;
  } catch(e) {
   let original=''+e,restore_error=null,restored=null;
   for(let c in reverse(changed))try{set_category(s,c,p.before[c.name]);}catch(err){restore_error=''+err;}
   try {
    restored=snapshot(s);
    for(let c in s.categories)if(!same(restored.offsets.values[c.name],p.before[c.name]))die('Power rollback mismatch');
    for(let key,v in before.power.rates)if(restored.power.rates[key]!=v)die('Firmware rollback mismatch');
   } catch(err){restore_error=''+err;restored=null;}
   let out;
   if(restored)out=result(s,cfg,restored,null,false,original,true);
   else out={ok:false,radio:radio,error:original+'; rollback: '+(restore_error??'unverified'),rollback_verified:false,reference_dbm:null,maximum_dbm:null,minimum_dbm:null,timestamp:now(),context:context(s,cfg),requested_dbm:M.requested(cfg.txpower)};
   store(radio,out);return out;
  }
 });
}
