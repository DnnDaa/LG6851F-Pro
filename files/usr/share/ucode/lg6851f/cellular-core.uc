// SPDX-License-Identifier: GPL-2.0-only
// Pure accounting and SMS parsing. No device I/O in this module.
'use strict';

export function number(v, fallback) {
 return match(''+v, /^-?[0-9]+(\.[0-9]+)?$/) ? +v : (fallback ?? 0);
}
export function month_days(y,m) {
 if (m==2) return (y%4==0 && (y%100!=0 || y%400==0)) ? 29 : 28;
 return index([4,6,9,11],m)>=0 ? 30 : 31;
}
function date_string(y,m,d) { return sprintf('%04d-%02d-%02d',y,m,d); }
export function cycle_for(date,day) {
 let f=split(date,'-'),y=+f[0],m=+f[1],d=+f[2];
 day=max(1,min(31,int(day)));
 let start_day=min(day,month_days(y,m));
 if(d<start_day) { m--; if(m<1){m=12;y--;} }
 let start=date_string(y,m,min(day,month_days(y,m)));
 let ny=y,nm=m+1;if(nm>12){nm=1;ny++;}
 let next_day=min(day,month_days(ny,nm));
 let next=date_string(ny,nm,next_day);
 let last_day=next_day-1,ly=ny,lm=nm;
 if(last_day<1){lm--;if(lm<1){lm=12;ly--;}last_day=month_days(ly,lm);}
 return {start:start,last:date_string(ly,lm,last_day),next:next};
}
function serial(date) {
 let f=split(date,'-'),y=+f[0],m=+f[1],d=+f[2];
 y-=m<=2 ? 1:0;
 let era=int(y/400),yoe=y-era*400;
 let doy=int((153*(m+(m>2?-3:9))+2)/5)+d-1;
 return era*146097+yoe*365+int(yoe/4)-int(yoe/100)+doy;
}
export function traffic_new(now) {
 return {version:1,since:now,days:{},adjustments:{},pending:{rx:0,tx:0},baseline:null,baselines:{},last_day:null,cut_cycle:null,notified:null,dirty:false};
}
export function traffic_sample(db,sample) {
 let rx=0,tx=0;
 if(sample.valid) {
  db.baselines=db.baselines ?? {};
  let prev=db.baselines[sample.device];
  if(prev) {
   let reset=prev.boot!=sample.boot || prev.ifindex!=sample.ifindex;
   rx=reset || sample.rx<prev.rx ? sample.rx : sample.rx-prev.rx;
   tx=reset || sample.tx<prev.tx ? sample.tx : sample.tx-prev.tx;
  }
  else if(db.baseline) {rx=sample.rx;tx=sample.tx;}
  // First installation starts here; after an ordinary reboot, previous
  // checkpointed boot ID lets us count the new boot's counters once.
  db.baseline={boot:sample.boot,device:sample.device,ifindex:sample.ifindex,rx:sample.rx,tx:sample.tx};
  db.baselines[sample.device]=db.baseline;
 }
 if(rx>0 || tx>0)db.dirty=true;
 let date=sample.date;
 if(!date || (db.last_day && date<db.last_day)) {
  db.pending.rx+=rx;db.pending.tx+=tx;return db;
 }
 db.last_day=date;
 if(!db.since || db.since<1735689600)db.since=sample.now;
 let row=db.days[date] ?? {rx:0,tx:0};
 row.rx+=rx+db.pending.rx;row.tx+=tx+db.pending.tx;db.pending={rx:0,tx:0};
 db.days[date]=row;
 // Keep about two years of daily totals, bounded to a small JSON file.
 let day_keys=sort(keys(db.days));
 while(length(day_keys)>740) { let old=shift(day_keys);delete db.days[old];delete db.adjustments[old]; }
 return db;
}
export function traffic_summary(db,date,settings) {
 let cycle=cycle_for(date,settings.cycle_day),list=[],cycles={},rx=0,tx=0;
 for(let day,entry in db.days) {
  push(list,{date:day,rx:entry.rx,tx:entry.tx});
  let c=cycle_for(day,settings.cycle_day),row=cycles[c.start] ?? {start:c.start,last:c.last,rx:0,tx:0};
  row.rx+=entry.rx;row.tx+=entry.tx;cycles[c.start]=row;
  if(day>=cycle.start && day<cycle.next){rx+=entry.rx;tx+=entry.tx;}
 }
 let adj=number(db.adjustments[cycle.start],0),used=max(0,rx+tx+adj+db.pending.rx+db.pending.tx);
 cycle.rx=rx;cycle.tx=tx;cycle.adj=adj;cycle.used=used;cycle.days_left=max(0,serial(cycle.next)-serial(date)-1);
 let cap=number(settings.cap_bytes,0),warn=number(settings.warn_percent,90);
 let level=cap<=0?'none':used>=cap?'over':used*100>=cap*warn?'warn':'ok';
 let history=[];for(let start,row in cycles) {row.adj=number(db.adjustments[start],0);row.used=max(0,row.rx+row.tx+row.adj);push(history,row);}
 sort(list,(a,b)=>a.date>b.date?-1:1);sort(history,(a,b)=>a.start>b.start?-1:1);
 return {ok:1,cycle:cycle,today:db.days[date] ?? {rx:0,tx:0},days:list,cycles:history,
  cap:{bytes:cap,warn:warn,level:level,cut_enabled:settings.cut_enabled==1,cut_active:!!db.cut_cycle},
  pending:db.pending,since:db.since,device:db.baseline?.device ?? null};
}
export function csv(text) {
 let out=[],value='',quoted=false;
 for(let i=0;i<length(text);i++) {
  let ch=substr(text,i,1);
  if(ch=='"')quoted=!quoted;
  else if(ch==',' && !quoted){push(out,trim(value));value='';}
  else value+=ch;
 }
 push(out,trim(value));return out;
}
export function ucs2(text) {
 if(!length(text) || length(text)%4 || !match(text,/^[0-9A-Fa-f]+$/))return text;
 let out='';
 for(let i=0;i<length(text);i+=4) {
  let point=int(substr(text,i,4),16);
  if(point>=0xd800 && point<=0xdbff && i+4<length(text)) {
   let low=int(substr(text,i+4,4),16);
   if(low>=0xdc00 && low<=0xdfff){point=0x10000+(point-0xd800)*1024+low-0xdc00;i+=4;}
  }
  if(point>=0xd800 && point<=0xdfff)point=0xfffd;
  out+=uchr(point);
 }
 return out;
}
export function sms_parse(raw,storage) {
 let rows=[],record=null;
 for(let line in split(replace(raw,'\r',''),'\n')) {
  let header=match(line,/^\+CMGL:\s*(.*)$/);
  if(header) {
   if(record)push(rows,record);
   let f=csv(header[1]);
   let peer=f[2]??'';
   record={index:int(f[0]),state:f[1],number:match(peer,/^(00[0-9A-Fa-f]{2})+$/)?ucs2(peer):peer,time:f[4]??'',body:'',storage:storage};
  }
  else if(record && trim(line)!='OK' && !match(line,/^(ERROR|\+CMS ERROR|\+CME ERROR)/)) record.body+=(length(record.body)?'\n':'')+line;
 }
 if(record)push(rows,record);
 for(let row in rows) {
  let hex=replace(trim(row.body),'\n',''),part=match(hex,/^050003([0-9A-Fa-f]{2})([0-9A-Fa-f]{2})([0-9A-Fa-f]{2})/);
  let wide=match(hex,/^060804([0-9A-Fa-f]{4})([0-9A-Fa-f]{2})([0-9A-Fa-f]{2})/);
  let meta=wide ?? part;
  if(meta){row.ref=meta[1];row.total=int(meta[2],16);row.part=int(meta[3],16);hex=substr(hex,wide?14:12);}
  row.text=ucs2(hex);delete row.body;
  row.direction=index(row.state??'','STO')==0?'out':'in';
  row.fingerprint=row.number+'|'+row.time+'|'+row.text;
 }
 return rows;
}
export function sms_new() {return {version:1,next:1,messages:[],hidden:[],synced:{},updated:0};}
export function sms_merge(pool,records,force) {
 let changed=false,existing={};
 for(let row in pool.messages)existing[row.fingerprint]=true;
 if(force && length(pool.hidden)){pool.hidden=[];changed=true;}
 for(let row in records) {
  if(!row.number || !length(row.text) || existing[row.fingerprint] || (!force && index(pool.hidden,row.fingerprint)>=0))continue;
  if(length(pool.messages)>=2000)break;
  row.id=''+pool.next++;row.read=row.direction=='out';row.received=time();
  push(pool.messages,row);existing[row.fingerprint]=true;changed=true;
 }
 return changed;
}
