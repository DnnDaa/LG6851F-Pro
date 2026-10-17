// SPDX-License-Identifier: GPL-2.0-only
'use strict';
import {readfile,open,rename,unlink,mkdir,chmod,glob,popen} from 'fs';
import {cursor} from 'uci';
import * as C from './cellular-core.uc';

const DATA='/etc/lg6851f-cellular';
const RAM='/tmp/lg6851fcell-traffic.json';
const DISK=DATA+'/traffic.json';
const POOL=DATA+'/sms.json';
const JOBS='/tmp/lg6851fcell-jobs';
const SELF='/usr/libexec/lg6851f-cellular';
function directories() {mkdir(DATA,0700);mkdir(JOBS,0700);chmod(DATA,0700);chmod(JOBS,0700);}
function quote(v) {return "'"+replace(''+v,"'","'\\''")+"'";}
function read_json(path,fallback) {
 let raw=readfile(path);if(raw==null)return fallback;
 if(length(raw)>8388608)die('Stored data exceeds the supported size');
 let value;try{value=json(raw);}catch(e){die('Stored data is damaged: '+path);}
 if(type(value)!='object')die('Invalid stored data: '+path);
 return value;
}
function atomic(path,data) {
 let f=open(path+'.new','w',0600);if(!f)die('Cannot write '+path);
 if(!f.write(sprintf('%J',data))){f.close();die('Write failed: '+path);}
 f.close();if(!rename(path+'.new',path))die('Cannot replace '+path);
 chmod(path,0600);
}
function with_lock(name,fn,wait) {
 directories();let guard=open('/tmp/lg6851fcell-'+name+'.lock','a',0600);
 if(!guard || !guard.lock(wait?'x':'xn'))return {ok:0,error:'操作正在进行，请稍后重试'};
 let result;try{result=fn();}catch(e){result={ok:0,error:''+e};}
 guard.lock('u');guard.close();return result;
}
function capture(argv,seconds) {
 let p=popen('/usr/bin/timeout '+(seconds??15)+' '+join(' ',map(argv,quote))+' 2>&1','r');
 let output=p?.read('all')??'',code=p?.close()??1;
 return {output:output,code:code};
}
function at(command) {return capture(['/usr/bin/modem5g-at',command],18);}
function at_ok(result) {return result.code==0 && !!match(result.output,/(^|\n)OK\s*(\n|$)/) && !match(result.output,/(\+CMS ERROR|\+CME ERROR|\nERROR)/);}
function today(now) {
 if(now<1735689600)return null;
 let t=localtime(now);return sprintf('%04d-%02d-%02d',t.year,t.mon,t.mday);
}
function settings() {
 let u=cursor();return {cycle_day:max(1,min(31,C.number(u.get('lg6851f_cellular','main','cycle_day'),1))),
 cap_bytes:max(0,C.number(u.get('lg6851f_cellular','main','cap_bytes'),0)),
 warn_percent:max(1,min(100,C.number(u.get('lg6851f_cellular','main','warn_percent'),90))),
 cut_enabled:u.get('lg6851f_cellular','main','cut_enabled')=='1'};
}
function traffic_db() {let ram=read_json(RAM,null);return ram ?? read_json(DISK,C.traffic_new(time()));}
function traffic_snapshot() {
 let now=time(),state=capture(['/bin/ubus','-S','call','network.interface.modem5g','status'],4),wan={};
 try{wan=json(state.output);}catch(e){}
 let name=wan.l3_device??wan.device;
 let result={valid:false,now:now,date:today(now),boot:trim(readfile('/proc/sys/kernel/random/boot_id')??''),device:name,rx:0,tx:0};
 if(!match(name??'',/^ccmni[0-9]+$/))return result;
 // One read only: individual ccmni sysfs byte counters each issue their own
 // slow modem query. Hardware-accelerated traffic must use CCMNI, not LAN.
 let cache={},entry=null;
 try{cache=read_json('/tmp/fiberhomedash.live.json',{});}catch(e){}
 if(cache.mobile_if==name && now-C.number(cache.ts,0)<=20)entry=cache.counters?.[name];
 if(entry){result.rx=C.number(entry.rx,0);result.tx=C.number(entry.tx,0);result.valid=true;}
 else for(let line in split(readfile('/proc/net/dev')??'','\n')) {
  let m=match(line,/^\s*([^:]+):\s*(.*)$/);if(!m || trim(m[1])!=name)continue;
  let fields=split(trim(m[2]),/\s+/);if(length(fields)<9)continue;
  result.rx=C.number(fields[0],0);result.tx=C.number(fields[8],0);result.valid=true;
 }
 result.ifindex=trim(readfile('/sys/class/net/'+name+'/ifindex')??'');
 return result;
}
export function traffic_get() {
 let db=traffic_db(),now=time(),date=today(now)??db.last_day;
 if(!date)return {ok:1,clock_valid:false,initializing:true,cycle:null,error:'系统时间尚未同步，流量增量暂存，时间恢复后自动展示。'};
 let out=C.traffic_summary(db,date,settings());out.clock_valid=!!today(now);out.updated=db.updated??0;
 out.settings=settings();out.available=!!db.available;out.checkpoint=db.checkpoint??0;
 out.collector=read_json('/tmp/lg6851fcell-traffic-health.json',{ok:0,error:'流量采集服务尚未启动'});
 out.collector.age=out.collector.updated?max(0,time()-out.collector.updated):null;
 return out;
}
function enforce_cap(db,date,config) {
 if(!date)return;
 let summary=C.traffic_summary(db,date,config),cycle=summary.cycle.start;
 // Only restore data that this feature itself disabled; never undo a manual
 // user disconnect, and never repeatedly toggle the modem at the cap.
 if(db.cut_cycle && (db.cut_cycle!=cycle || !config.cut_enabled || summary.cap.level!='over')) {
  let r=capture(['/sbin/ifup','modem5g'],5);if(r.code==0)db.cut_cycle=null;
 }
 if(config.cut_enabled && summary.cap.level=='over' && !db.cut_cycle) {
  let r=capture(['/sbin/ifdown','modem5g'],5);if(r.code==0)db.cut_cycle=cycle;
 }
 let warning=summary.cap.level=='warn'||summary.cap.level=='over'?cycle+':'+summary.cap.level:null;
 if(warning && db.notified!=warning) {
  capture(['/usr/bin/logger','-t','lg6851f-traffic','Cellular data usage '+summary.cap.level+': '+summary.cycle.used+' / '+summary.cap.bytes],3);
  db.notified=warning;
 }
}
export function traffic_tick(flush) {
 return with_lock('traffic',function(){
  let db=traffic_db(),sample=traffic_snapshot(),before=sprintf('%J',db.days);
  C.traffic_sample(db,sample);db.available=sample.valid;db.updated=sample.now;
  enforce_cap(db,sample.date,settings());
 let changed=db.dirty || before!=sprintf('%J',db.days) || db.pending.rx>0 || db.pending.tx>0;
  let mono=clock(true)[0];
  if(flush || db.checkpoint_boot!=sample.boot || mono-C.number(db.checkpoint_mono,0)>=1800) {
   if(changed || db.checkpoint_boot!=sample.boot || flush){
    db.checkpoint=sample.now;db.checkpoint_boot=sample.boot;db.checkpoint_mono=mono;
    db.dirty=false;atomic(DISK,db);
   }
  }
  atomic(RAM,db);return {ok:1};
 },flush);
}
export function traffic_set(op,value) {
 return with_lock('traffic',function(){
  let db=traffic_db(),config=settings(),u=cursor(),now=time(),date=today(now)??db.last_day;
  if(!date)return {ok:0,error:'系统时间尚未同步，请稍后设置'};
  let n=C.number(value,-1);
  if(op=='reset') {
   let baseline=db.baseline,baselines=db.baselines,cut=db.cut_cycle;
   db=C.traffic_new(now);db.baseline=baseline;db.baselines=baselines;db.cut_cycle=cut;db.last_day=date;
  }
  else if(op=='used') {
   if(n<0 || n>1073741824000000)return {ok:0,error:'用量超出范围'};
   let c=C.traffic_summary(db,date,config).cycle;
   db.adjustments[c.start]=n-c.rx-c.tx-db.pending.rx-db.pending.tx;
  }
  else {
   let range={cycle_day:[1,31],cap_bytes:[0,1073741824000000],warn_percent:[1,100],cut_enabled:[0,1]};
   if(!range[op] || n<range[op][0] || n>range[op][1] || n!=int(n))return {ok:0,error:'设置值无效'};
   u.set('lg6851f_cellular','main',op,''+n);if(!u.commit('lg6851f_cellular'))return {ok:0,error:'配置保存失败'};
  }
  enforce_cap(db,date,settings());db.checkpoint=now;db.checkpoint_mono=clock(true)[0];
  db.checkpoint_boot=trim(readfile('/proc/sys/kernel/random/boot_id')??'');db.dirty=false;
  atomic(DISK,db);atomic(RAM,db);return {ok:1};
 });
}
export function sms_get(revision) {
 let pool=read_json(POOL,C.sms_new()),job=read_json('/tmp/lg6851fcell-sms-status.json',{});
 let rev=''+pool.updated+':'+pool.next+':'+length(pool.messages);
 return {ok:1,messages:revision==rev?null:pool.messages,not_modified:revision==rev,revision:rev,updated:pool.updated,synced:pool.synced,job:job,capacity:2000};
}
export function sms_delete(id,clear) {
 return with_lock('sms',function(){
  let pool=read_json(POOL,C.sms_new()),retained=[],found=false;
  for(let row in pool.messages) {
   if(clear || row.id==id){if(index(pool.hidden,row.fingerprint)<0)push(pool.hidden,row.fingerprint);found=true;}
   else push(retained,row);
  }
  if(!found)return {ok:1};
  // Tombstones prevent the auto-refresh from immediately importing deleted
  // SIM/device messages again. Explicit SIM sync can import them again.
  if(length(pool.hidden)>4000)return {ok:0,error:'本地删除标记已达上限，请先从 SIM 同步后再清理'};
  pool.messages=retained;pool.updated=time();atomic(POOL,pool);return {ok:1};
 });
}
function sms_sync(storage,force) {
 let pool=read_json(POOL,C.sms_new()),previous=at('AT+CPMS?'),old='';
 let m=match(previous.output,/\+CPMS:\s*"([A-Z]+)"/);if(m)old=m[1];
 if(!old)return {ok:0,error:'无法读取原短信存储位置，未更改模组设置'};
 if(storage=='AUTO')storage=old;
 let mode=at('AT+CMGF?'),charset=at('AT+CSCS?');
 let old_mode=match(mode.output,/\+CMGF:\s*([01])/),old_charset=match(charset.output,/\+CSCS:\s*"([A-Za-z0-9_-]+)"/);
 if(!old_mode || !old_charset)return {ok:0,error:'无法读取短信模式，未更改模组设置'};
 let selected=at('AT+CPMS="'+storage+'"'),formatted=at('AT+CMGF=1'),encoded=at('AT+CSCS="UCS2"'),result={ok:0,error:'短信同步失败'};
 try {if(at_ok(selected) && at_ok(formatted) && at_ok(encoded)) {
  let r=at('AT+CMGL="ALL"');
  if(at_ok(r)) {
   let rows=C.sms_parse(r.output,storage),changed=C.sms_merge(pool,rows,force);
   pool.synced[storage]=time();
   if(changed){pool.updated=time();atomic(POOL,pool);}
   // Store sync time in RAM when no new message exists, to avoid idle writes.
   result={ok:1,storage:storage,found:length(rows),total:length(pool.messages),full:length(pool.messages)>=2000};
  }
  else result.error='模组未确认短信读取：'+substr(trim(r.output),0,160);
 }}catch(e){result={ok:0,error:''+e};}
 let restored=[at('AT+CPMS="'+old+'"'),at('AT+CSCS="'+old_charset[1]+'"'),at('AT+CMGF='+old_mode[1])];
 if(!at_ok(restored[0]) || !at_ok(restored[1]) || !at_ok(restored[2]))
  result={ok:0,error:'同步结束，但模组未确认恢复原短信设置；请稍后刷新',imported:result.total??null};
 return result;
}
function sms_send(number,text) {
 let pool=read_json(POOL,C.sms_new());
 if(length(pool.messages)>=2000)return {ok:0,error:'短信本地池已满，请先清理'};
 // Preserve this board's verified stock libqlril transport. AT+CMGS body
 // mode is unreliable through its atcid queue; never silently fall back.
 if(!match(number,/^\+?[0-9]{3,20}$/) || !match(text,/^[\x20-\x7e\r\n]{1,160}$/))
  return {ok:0,error:'当前原厂发送接口支持 1–160 个 ASCII 可打印字符'};
 let r=capture(['/usr/bin/modem5g-sms-send',number,text],25);
 if(r.code!=0 || index(r.output,'official qlril API')<0)return {ok:0,error:'模组未确认短信提交，请检查后再手动发送',detail:substr(trim(r.output),0,160)};
 let stamp=localtime(time());
 let row={id:''+pool.next++,number:number,text:text,direction:'out',time:sprintf('%04d-%02d-%02d %02d:%02d:%02d',stamp.year,stamp.mon,stamp.mday,stamp.hour,stamp.min,stamp.sec),received:time(),read:true,status:'已提交模组',fingerprint:'sent|'+time()+'|'+number+'|'+text};
 push(pool.messages,row);pool.updated=time();atomic(POOL,pool);return {ok:1,id:row.id,status:'已提交模组'};
}
export function job_get(id) {
 if(!match(id??'',/^[0-9]+-[0-9]+$/))return {ok:0,error:'无效任务'};
 return read_json(JOBS+'/'+id+'.result',{ok:1,pending:true});
}
export function queue(op,args) {
 directories();
 if(index(['sync','send'],op)<0)return {ok:0,error:'无效操作'};
 let status=read_json('/tmp/lg6851fcell-sms-status.json',{});
 if(status.busy && time()-C.number(status.started,0)<120)return {ok:0,error:'短信操作正在进行'};
 if(op=='sync' && !args.force && time()-C.number(status.synced_me,0)<30)return {ok:1,cached:true};
 if(op=='sync' && index(['AUTO','ME','SM'],args.storage)<0)return {ok:0,error:'无效存储位置'};
 if(op=='send' && (!match(args.number??'',/^\+?[0-9]{3,20}$/) || !match(args.text??'',/^[\x20-\x7e\r\n]{1,160}$/)))
  return {ok:0,error:'当前原厂发送接口支持 1–160 个 ASCII 可打印字符'};
 let pid=split(readfile('/proc/self/stat')??'0',' ')[0],id=''+time()+'-'+pid+sprintf('%09d',clock(true)[1]);
 for(let path in glob(JOBS+'/*')??[]) {
  let name=substr(path,length(JOBS)+1),stamp=C.number(split(name,'-')[0],0);
  if(time()-stamp>600)unlink(path);
 }
 atomic(JOBS+'/'+id+'.request',{op:op,args:args});
 system('PATH=/bin:/usr/bin:/sbin:/usr/sbin; export PATH; setsid nice -n 10 ucode '+quote(SELF)+' sms-job '+quote(id)+' >/dev/null 2>&1 </dev/null &');
 return {ok:1,id:id,pending:true};
}
export function job_run(id) {
 if(!match(id??'',/^[0-9]+-[0-9]+$/))return;
 let request=read_json(JOBS+'/'+id+'.request',null);if(!request)return;
 let result=with_lock('sms',function(){
  let status=read_json('/tmp/lg6851fcell-sms-status.json',{});
  status.busy=true;status.started=time();status.id=id;atomic('/tmp/lg6851fcell-sms-status.json',status);
  let r;try{r=request.op=='sync'?sms_sync(request.args.storage,request.args.force):sms_send(request.args.number,request.args.text);}catch(e){r={ok:0,error:''+e};}
  status.busy=false;status.error=r.ok?null:r.error;status.finished=time();
  if(r.ok && request.op=='sync' && request.args.storage!='SM')status.synced_me=time();
  atomic('/tmp/lg6851fcell-sms-status.json',status);return r;
 });
 atomic(JOBS+'/'+id+'.result',result);unlink(JOBS+'/'+id+'.request');
}
