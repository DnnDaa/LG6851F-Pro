// SPDX-License-Identifier: GPL-2.0-only
'use strict';
function number(s) {
 if (s == null || !match(trim(''+s), /^[0-9]+$/)) return null;
 return int(s);
}
function valid(n,max) { return n != null && n >= 0 && n <= max; }
function metric(v) { let n=number(v); return n==255 || n==65535 ? null : n; }
/* This firmware's NR code 11 was cross-checked with the original band getter
 * returning 100M. Other codes are learned only from this same vendor getter,
 * matching RAT and primary ARFCN. Do not borrow Qualcomm/other AT enums. */
function bandwidth(rat,code,maps) {
 if(code==null || code==255 || code==65535) return null;
 return maps?.[rat]?.[''+code] ?? null;
}
export function parse_ca(text, previous_maps) {
 if(index(text??'','get all info success!')<0) return null;
 let blocks=[],current=null,band={},maps={nr:{'11':100},lte:{}};
 for(let rat in ['nr','lte']) for(let key,value in (previous_maps?.[rat]??{}))
  if(match(key,/^[0-9]+$/)&&value>0&&value<=400) maps[rat][key]=value;
 for(let line in split(text??'','\n')) {
  let m=match(trim(line),/^(pcc|scc)_([a-z0-9_]+)[[:space:]]*=[[:space:]]*(.*)$/);
  if(m) {
   if(m[2]=='type') { current={role:m[1],values:{}};push(blocks,current); }
   if(current && current.role==m[1]) current.values[m[2]]=trim(m[3]);
  }
  // The sample's numbered log prefix precedes nr/lte fields.
  let b=match(line,/(nr|lte)_([a-z_]+)[[:space:]]*=[[:space:]]*([0-9]+)(M)?[[:space:]]*$/);
  if(b) { band[b[1]]=band[b[1]]??{};band[b[1]][replace(b[2],'earfcn','arfcn')]=number(b[3]); }
 }
 if(!length(blocks)||length(blocks)>32||blocks[0].role!='pcc') return null;
 let carriers=[],seen={},pcc_count=0;
 for(let block in blocks) {
  let v=block.values,t=match(v.type??'',/^(NR|LTE) (PCC|SCC)/);
  if(!t || (block.role=='pcc' && t[2]!='PCC') || (block.role=='scc'&&t[2]!='SCC')) return null;
  let rat=t[1]=='NR'?'nr':'lte',arfcn=number(v.arfcn),pci=number(v.pci),freqband=number(v.band);
  if(!valid(arfcn,rat=='nr'?3279165:262143)||!valid(pci,rat=='nr'?1007:503)||!freqband||freqband>1024) return null;
  let dl_code=number(v.dl_bandwidth),ul_code=number(v.ul_bandwidth),key=rat+':'+arfcn;
  if(block.role=='pcc') {
   pcc_count++;
   let b=band[rat];
   if(b?.dl_arfcn==arfcn && b?.band==freqband && dl_code!=null && dl_code<255 && b?.dl_bandwidth>0 && b.dl_bandwidth<=400)
    maps[rat][''+dl_code]=b.dl_bandwidth;
   if(b?.dl_arfcn==arfcn && b?.band==freqband && ul_code!=null && ul_code<255 && b?.ul_bandwidth>0 && b.ul_bandwidth<=400)
    maps[rat][''+ul_code]=b.ul_bandwidth;
  }
  if(seen[key]) {
   let old=seen[key];
   if(old.pci!=pci || old.band!=freqband || old.dl_bandwidth_code!=dl_code) return null;
   continue; // repeated native records cannot inflate the total
  }
  let c={role:block.role,serving:block.role=='pcc'?1:0,rat:rat,band:freqband,arfcn:arfcn,pci:pci,
   state:metric(v.state),ul_configured:metric(v.ul_configured),
   dl_bandwidth_code:dl_code,ul_bandwidth_code:ul_code,
   dl_mcs:metric(v.dl_mcs),ul_mcs:metric(v.ul_mcs),
   dl_rb:metric(v.dl_rb),ul_rb:metric(v.ul_rb),
   dl_mimo_code:metric(v.dl_mimo),ul_mimo_code:metric(v.ul_mimo),
   dl_modulation_code:metric(v.dl_modulation),ul_modulation_code:metric(v.ul_modulation)};
  seen[key]=c;push(carriers,c);
 }
 if(pcc_count!=1) return null;
 let total=0,complete=true;
 for(let c in carriers) {
  c.dl_bw_mhz=bandwidth(c.rat,c.dl_bandwidth_code,maps);
  c.ul_bw_mhz=c.ul_configured==0?null:bandwidth(c.rat,c.ul_bandwidth_code,maps);
  if(c.dl_bw_mhz==null)complete=false;else total+=c.dl_bw_mhz;
 }
 return {source:'vendor_qlril',carriers:carriers,bandwidth_maps:maps,dl_total_mhz:complete?total:null,
  dl_known_mhz:total,bandwidth_complete:complete?1:0,count:length(carriers)};
}
export function ca_view(cache,now,registered,offline,serving,serving_ts,allow_cached) {
 let age=cache?.ts!=null?now-cache.ts:null;
 let empty={source:'vendor_qlril',valid:0,reason:'pending',age:age,ts:cache?.ts??null,carriers:[],dl_total_mhz:null,count:0};
 if(!registered||offline) {empty.reason='offline';return empty;}
 if(age==null||age<0||(!allow_cached&&age>90)) {empty.reason=age==null?'pending':'stale';return empty;}
 let p=cache?.carriers?.[0];
 if(!p||p.role!='pcc') {empty.reason='pending';return empty;}
 if(serving?.serving && (serving_ts==null||serving_ts>cache.ts) && (serving.rat!=p.rat || serving.arfcn!=p.arfcn || serving.pci!=p.pci)) {
  empty.reason='serving_changed';return empty;
 }
 return {source:cache.source,valid:1,reason:null,ts:cache.ts,age:age,count:cache.count,
  dl_total_mhz:cache.dl_total_mhz,dl_known_mhz:cache.dl_known_mhz,
  bandwidth_complete:cache.bandwidth_complete,carriers:cache.carriers};
}

export function ca_refresh_due(now,attempt){return attempt==null || now<attempt || now-attempt>=25;}

// 3GPP TS 38.104 table 5.4.2.1-1. Never assume every NR cell is n78.
export function nr_arfcn_mhz(value) {
 let n=number(value);if(!valid(n,3279165))return null;
 if(n<600000)return n*0.005;
 if(n<2016667)return 3000+(n-600000)*0.015;
 return 24250.08+(n-2016667)*0.06;
}

// A read-only optimization: reuse native bandwidth only for this boot/SIM/RAT
// and exactly the measured primary cell. A retained display is not fresh I/O.
export function ca_primary_bandwidth(cache,scope,serving,now) {
 let stamp=number(cache?.ts),current=number(now);
 let age=stamp!=null&&current!=null?current-stamp:null;
 if(type(scope)!='string' || !length(scope) || cache?.radio_context!=scope || age==null || age<0 || age>90)return null;
 if(!serving?.serving || serving.rat!='nr')return null;
 if(!valid(number(serving.arfcn),3279165)||!valid(number(serving.pci),1007))return null;
 let p=cache?.carriers?.[0];
 if(p?.role!='pcc' || p.rat!='nr' || number(p.arfcn)!=number(serving.arfcn) || number(p.pci)!=number(serving.pci))return null;
 let band=number(serving.band);
 if(!valid(band,1024) || band==0 || number(p.band)!=band)return null;
 let bw=number(p.dl_bw_mhz);
 if(bw==null || bw<=0 || bw>100)return null;
 return {mhz:bw,ts:stamp,source:'vendor_qlril'};
}
