// SPDX-License-Identifier: GPL-2.0-only
'use strict';

const CATEGORIES = [
 ['PowerBoostCCK','CCK',4],['PowerBoostOFDM','OFDM',8],
 ['PowerBoostHT20','HT20',8],['PowerBoostHT40','HT40',9],
 ['PowerBoostVHT20','VHT20',12],['PowerBoostVHT40','VHT40',12],
 ['PowerBoostVHT80','VHT80',12],['PowerBoostVHT160','VHT160',12],
 ['PowerBoostHE26','HE26',12],['PowerBoostHE52','HE52',12],
 ['PowerBoostHE106','HE106',12],['PowerBoostHE242','HE242',12],
 ['PowerBoostHE484','HE484',12],['PowerBoostHE996','HE996',12],
 ['PowerBoostHE996X2','HE996X2',12],['PowerBoostEHT26','EHT26',16],
 ['PowerBoostEHT52','EHT52',16],['PowerBoostEHT106','EHT106',16],
 ['PowerBoostEHT242','EHT242',16],['PowerBoostEHT484','EHT484',16],
 ['PowerBoostEHT996','EHT996',16],['PowerBoostEHT996X2','EHT996X2',16],
 ['PowerBoostEHT996X4','EHT996X4',16],['PowerBoostEHT26_52','EHT26_52',16],
 ['PowerBoostEHT26_106','EHT26_106',16],['PowerBoostEHT484_242','EHT484_242',16],
 ['PowerBoostEHT996_484','EHT996_484',16],['PowerBoostEHT996_484_242','EHT996_484_242',16],
 ['PowerBoostEHT996X2_484','EHT996X2_484',16],['PowerBoostEHT996X3','EHT996X3',16],
 ['PowerBoostEHT996X3_484','EHT996X3_484',16]
];

export function spec(radio) {
 let ids, iface, band, reference;
 if (radio == 'radio0') {
  ids=[0,1,2,3,8,9,10,11,12,15,16,17,18,19];
  iface='ra0';band=0;reference='CCK_01M';
 } else if (radio == 'radio1') {
  ids=[1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21];
  iface='rai0';band=1;reference='OFDM_06M';
 } else die('Invalid radio');
 return {radio:radio,iface:iface,channel_band:band,reference:reference,
  categories:map(ids,id=>({id:id,name:CATEGORIES[id][0],prefix:CATEGORIES[id][1],size:CATEGORIES[id][2]}))};
}

export function requested(value) {
 if(value==null || value=='' || value=='auto')return null;
 if(!match(''+value,/^(0|[1-9][0-9]?)$/) || +value>30)die('Power target must be an integer from 0 to 30 dBm');
 return +value;
}

export function fingerprint(data) {
 if(type(data)!='object' || type(data.config)!='object')die('Invalid wireless configuration');
 delete data.config.txpower;
 return sprintf('%J',data);
}

// printk timestamps and /proc/uptime may use different clock domains.
// Accept only log records appended after the exact pre-command tail record.
export function appended_logs(before,after) {
 let prior=split(before,'\n');
 // Remove separator-only empty elements, never whitespace within a record.
 // TxPowerBoostCtrl emits rate lines ending in two spaces.
 while(length(prior) && prior[length(prior)-1]=='')pop(prior);
 let anchor=prior[length(prior)-1];
 if(!anchor || !match(anchor,/^\[\s*[0-9.]+\]/))die('Cannot establish driver log cursor');
 let rows=split(after,'\n'),pos=-1;
 for(let i=0;i<length(rows);i++)if(rows[i]==anchor)pos=i;
 if(pos<0)die('Driver response log cursor was overwritten');
 return join('\n',slice(rows,pos+1));
}

function body(line,since) {
 let stamp=match(line,/^\[\s*([0-9.]+)\]\s*/);
 if(!stamp || +stamp[1]<since)return null;
 return substr(line,length(stamp[0]));
}

export function parse_offsets(log,since) {
 log=replace(log,/\x1b\[[0-9;]*m/g,'');
 let result=null, name=null, wanted=0;
 for(let line in split(log,'\n')) {
  let s=body(line,since);if(s==null)continue;
  let heading=match(s,/Power Up Table \(Band([0-9]+)\)/);
  if(heading){result={band:+heading[1],values:{}};name=null;continue;}
  if(!result)continue;
  let category=match(s,/^(PowerBoost[A-Za-z0-9_]+):$/);
  if(category){name=category[1];wanted=0;for(let c in CATEGORIES)if(c[0]==name)wanted=c[2];continue;}
  s=trim(s);
  if(name && wanted && match(s,/^-?[0-9]+(\s+-?[0-9]+)*$/)) {
   let values=map(split(replace(s,/\s+/g,' '),' '),v=>+v);
   if(length(values)!=wanted || length(filter(values,v=>v < -63 || v > 63)))die('Invalid power offset table');
   result.values[name]=values;name=null;
  }
 }
 if(!result)die('Fresh driver power offset table unavailable');
 return result;
}

export function parse_rates(log,since,expected_band) {
 let result=null, candidate=null;
 for(let line in split(log,'\n')) {
  let s=body(line,since);if(s==null)continue;
  if(index(s,'TX POWER INFO')>=0){candidate={rates:{},band:null,channel_band:null};continue;}
  if(!candidate)continue;
  let band=match(s,/Band Index:\s*([0-9]+),\s*Channel Band:\s*([0-9]+):/);
  if(band){candidate.band=+band[1];candidate.channel_band=+band[2];continue;}
  let m;
  while((m=match(s,/\[([A-Z0-9_]+)\]:\s*0x[0-9a-fA-F]+\s*\((-?[0-9]+)\)/))) {
   candidate.rates[m[1]]=+m[2];
   s=substr(s,index(s,m[0])+length(m[0]));
  }
  if(candidate.channel_band==expected_band && length(candidate.rates))result=candidate;
 }
 if(!result || result.band==null)die('Fresh firmware power table unavailable');
 return result;
}

function key_for(c,index) {
 if(c.id==0)return ['CCK_01M','CCK_02M','CCK_05M','CCK_11M'][index];
 if(c.id==1)return ['OFDM_06M','OFDM_09M','OFDM_12M','OFDM_18M','OFDM_24M','OFDM_36M','OFDM_48M','OFDM_54M'][index];
 if(c.id==3 && index==8)return 'HT40_M32';
 return sprintf('%s_M%02d',c.prefix,index);
}

export function profile_defaults(text,s) {
 let raw={},result={};
 for(let line in split(text??'','\n')) {
  let m=match(trim(line),/^(PowerBoost[A-Za-z0-9_]+)=(.*)$/);
  if(m)raw[m[1]]=m[2];
 }
 for(let c in s.categories) {
  let values=[];
  if(raw[c.name]!=null && length(raw[c.name])) {
   let tokens=split(raw[c.name],':');
   if(length(tokens)>c.size)die('Invalid factory power boost profile');
   for(let v in tokens){if(!match(v,/^-?[0-9]+$/) || +v < -63 || +v > 63)die('Invalid factory power boost value');push(values,+v);}
  }
  while(length(values)<c.size)push(values,0);
  result[c.name]=values;
 }
 return result;
}

export function validate_snapshot(s,snapshot) {
 if(snapshot.offsets.band!=snapshot.power.band || snapshot.power.channel_band!=s.channel_band)die('Radio power table mismatch');
 for(let c in s.categories) {
  if(length(snapshot.offsets.values[c.name]??[])!=c.size)die('Incomplete driver power offset table: '+c.name);
  for(let i=0;i<c.size;i++) {
   let key=key_for(c,i),v=snapshot.power.rates[key];
   if(type(v)!='int' || v < -128 || v > 127)die('Incomplete firmware power table: '+key);
  }
 }
 let ref=snapshot.power.rates[s.reference];
 if(ref==126 || ref==127 || ref==-128)die('Invalid firmware reference power');
 return snapshot;
}

export function plan(s,snapshot,defaults,target) {
 validate_snapshot(s,snapshot);target=requested(target);
 let peak=-128, factory={}, before={}, planned={}, keys={}, clipped=0, preserved=0;
 for(let c in s.categories) {
  before[c.name]=[];
  for(let i=0;i<c.size;i++) {
   let key=key_for(c,i),v=snapshot.power.rates[key];
   push(before[c.name],snapshot.offsets.values[c.name][i]);
   if(v==126 || v==127 || v==-128)continue;
   let baseline=v-snapshot.offsets.values[c.name][i],f=baseline+defaults[c.name][i];
   if(f < -127 || f > 125)die('Factory rate target outside supported range');
   factory[key]=f;keys[key]={category:c.name,index:i,baseline:baseline};
   if(f>peak)peak=f;
  }
 }
 if(peak==-128)die('Factory power baseline unavailable');
 // mt76-style RATE limiting: each rate is independently bounded.
 // The board-specific factory target is our available per-rate limit.
 // Do not import mt76 chain corrections into this vendor firmware API.
 let delta=target==null || target*2>=peak?0:target*2-peak;
 for(let c in s.categories) {
  planned[c.name]=[];
  for(let i=0;i<c.size;i++) {
   let key=key_for(c,i),v=defaults[c.name][i];
   if(keys[key]!=null) {
    let limited=target==null?factory[key]:min(factory[key],target*2);
    v=limited-keys[key].baseline;
    if(limited<factory[key])clipped++;else preserved++;
   }
   // Unsupported/sentinel rates retain their original profile offset.
   if(v < -63 || v > 63)die('Requested rate limit exceeds compensation range');
   push(planned[c.name],v);
  }
 }
 let expected={};for(let key,item in keys) {
  let value=item.baseline+planned[item.category][item.index];
  if(value < -127 || value > 125)die('Predicted firmware power is outside the supported range');
  expected[key]=value;
 }
 return {target:target,delta:delta,factory_peak:peak,effective_peak:peak+delta,calibration_limited:target!=null && target*2>peak,factory:factory,clipped_rates:clipped,preserved_rates:preserved,before:before,values:planned,expected:expected};
}

export function verify(s,snapshot,p) {
 validate_snapshot(s,snapshot);
 for(let c in s.categories)for(let i=0;i<c.size;i++)
  if(snapshot.offsets.values[c.name][i]!=p.values[c.name][i])die('Driver did not apply power compensation: '+c.name);
 for(let key,want in p.expected)if(snapshot.power.rates[key]!=want)
  die(sprintf('Firmware limited %s: requested %.1f dBm, read back %.1f dBm',key,want/2.0,snapshot.power.rates[key]/2.0));
 return true;
}

export function summary(s,snapshot) {
 validate_snapshot(s,snapshot);let low=128,high=-128,count=0;
 for(let c in s.categories)for(let i=0;i<c.size;i++) {
  let v=snapshot.power.rates[key_for(c,i)];if(v==126 || v==127 || v==-128)continue;
  if(v<low)low=v;if(v>high)high=v;count++;
 }
 return {reference_dbm:snapshot.power.rates[s.reference]/2.0,minimum_dbm:low/2.0,maximum_dbm:high/2.0,rate_entries:count,firmware_band:snapshot.power.band};
}

// Compact readback for the active SU bandwidth. No synthetic NSS power values.
export function mcs_tables(s,snapshot,p,htmode) {
 let m=match(htmode??'',/^(HT|VHT|HE|EHT)(20|40|80|160)$/);
 if(!m)return [];
 let bw=+m[2],suffix=({'20':'242','40':'484','80':'996','160':'996X2'})[''+bw],out=[];
 for(let family in ['HE','EHT']) {
  let prefix=family+suffix,c=null;
  for(let item in s.categories)if(item.prefix==prefix)c=item;
  if(c==null)continue;
  let live=[],factory=[];
  for(let i=0;i<c.size;i++) {
   let key=key_for(c,i),v=snapshot.power.rates[key];
   push(live,p.factory[key]==null?null:v/2.0);
   push(factory,p.factory[key]==null?null:p.factory[key]/2.0);
  }
  push(out,{mode:family+bw,values_dbm:live,factory_limits_dbm:factory});
 }
 return out;
}
