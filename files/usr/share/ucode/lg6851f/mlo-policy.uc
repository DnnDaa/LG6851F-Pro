'use strict';
import {readfile,open,rename,access} from 'fs';
export const MAX_BSS=8;
function vendor_mlo_enabled(ap){return (ap.vendor_mlo??ap.mlo)=='1';}
function vendor_protocol(ap,dev){
 let p=ap.vendor_protocol;
 if(p=='be'||p=='ax'||p=='ac'||p=='n')return p;
 return match(dev?.htmode??'',/^EHT(20|40|80|160)([+-])?$/)?'be':'ax';
}
function active(u,s){return s.mode=='ap' && s.disabled!='1' && u.get('wireless',s.device,'disabled')!='1';}
function stable_slots(u,radio,active_aps){
 let path='/tmp/lg6851f-vendor618/bss-slots-'+radio+'.json',state={};
 try{let raw=json(readfile(path)??'{}');if(type(raw)=='object')state=raw;}catch(e){}
 let previous=type(state[radio])=='object'?state[radio]:{};
 let assigned={},used={};
 // Keep every surviving section on its current native MBSS slot. A middle
 // deletion therefore leaves a hole instead of renumbering unrelated APs.
 for(let item in active_aps){
  let slot=previous[item.section];
  if(type(slot)=='int'&&slot>=0&&slot<MAX_BSS&&!used[''+slot]){
   assigned[item.section]=slot;used[''+slot]=true;
  }
 }
 // Deleted sections release their slot. The disabled native object is reused
 // in place, so eight is a concurrent-AP limit rather than a lifetime limit.
 for(let item in active_aps){
  if(assigned[item.section]!=null)continue;
  let slot=0;while(used[''+slot]&&slot<MAX_BSS)slot++;
  if(slot>=MAX_BSS)die('At most eight enabled APs are supported per radio');
  assigned[item.section]=slot;used[''+slot]=true;
 }
 let changed=false;
 if(type(state[radio])!='object'||length(keys(previous))!=length(keys(assigned)))changed=true;
 if(!changed)for(let section,slot in assigned)if(previous[section]!=slot){changed=true;break;}
 if(changed){
  state[radio]=assigned;
  let fd=open(path+'.new','w',0600);
  if(!fd)die('Cannot save stable BSS slot map');
  fd.write(sprintf('%J',state));fd.close();
  if(!rename(path+'.new',path))die('Cannot publish stable BSS slot map');
 }
 return assigned;
}
export function bss_plan(u,radio){
 let dev=u.get_all('wireless',radio),aps=[],prefix=radio=='radio0'?'ra':'rai';
 if(dev?.type!='vendor618')return [];
 u.foreach('wireless','wifi-iface',function(s){
  if(s.device!=radio||!active(u,s))return;
  if(!match(s['.name'],/^[A-Za-z0-9_]+$/))die('Invalid BSS section identity');
  push(aps,{section:s['.name'],config:s});
 });
 let slots=stable_slots(u,radio,aps),out=[];
 for(let item in aps){
  let index=slots[item.section];
  push(out,{section:item.section,index:index,ifname:prefix+index,config:item.config});
 }
 // Netifd and the native driver both consume the layout in slot order.
 sort(out,(a,b)=>a.index-b.index);
 return out;
}
function group_id(u,ssid){
 // Keep existing IDs stable when another MLO network is added or removed.
 let path='/tmp/lg6851f-vendor618/mlo-groups.json',ids={};
 try{let v=json(readfile(path)??'{}');if(type(v)=='object')ids=v;}catch(e){}
 let wanted={},live={};
 for(let radio in ['radio0','radio1'])for(let b in bss_plan(u,radio))
  if(vendor_mlo_enabled(b.config))wanted[b.config.ssid]=true;
 for(let prefix in ['ra','rai'])for(let slot=0;slot<MAX_BSS;slot++){
  let dev=prefix+slot,flags=readfile('/sys/class/net/'+dev+'/flags');
  if(flags!=null&&!(int(trim(flags),16)&1))continue;
  if(flags==null&&!access('/sys/class/net/'+dev))continue;
  let band=prefix=='ra'?'24':'5',conf='/tmp/lg6851f-vendor618/hostapd'+band+(slot?'-'+slot:'')+'.conf';
  let raw=readfile(conf+'.mld_applied');
  // An interrupted setup may have opened a link before recording its group.
  // Reserve all recorded IDs in that case rather than guessing and reusing one.
  if(raw==null||!match(trim(raw),/^[0-9]*$/)){
   for(let name,id in ids)live[''+id]=true;
   continue;
  }
  let id=int(trim(raw));
  if(id>=1&&id<=16)live[''+id]=true;
 }
 for(let name,id in ids)if(!wanted[name]&&!live[''+id])delete ids[name];
 let used={};
 for(let name,id in ids)if(type(id)=='int'&&id>=1&&id<=16)used[''+id]=true;else delete ids[name];
 if(ids[ssid]!=null)return ids[ssid];
 let next=1;while(used[''+next]&&next<=16)next++;
 if(next>16)die('MLO group capacity reached; restart wireless configuration from a fresh boot');
 ids[ssid]=next;
 let fd=open(path+'.new','w',0600);if(!fd)die('Cannot allocate native MLD identity');
 fd.write(sprintf('%J',ids));fd.close();if(!rename(path+'.new',path))die('Cannot save native MLD identity');
 return next;
}
export function mlo_group(u,radio,section){
 let ap=null,dev=u.get_all('wireless',radio);
 for(let b in bss_plan(u,radio))if(section==null||section==b.section){ap=b.config;break;}
 if(!ap||!vendor_mlo_enabled(ap)||dev?.disabled=='1')return {enabled:false,group:0};
 if(vendor_protocol(ap,dev)!='be')return {enabled:false,group:0};
 let peers=[],per_radio={};
 for(let band in ['radio0','radio1'])for(let b in bss_plan(u,band)){
  let peer=b.config;if(!vendor_mlo_enabled(peer)||peer.ssid!=ap.ssid)continue;
  let peerdev=u.get_all('wireless',band);
  if(vendor_protocol(peer,peerdev)!='be')continue;
  if(per_radio[band])die('An MLD can contain only one affiliated BSS from each radio');
  per_radio[band]=true;
  push(peers,{band:band,ap:peer});
 }
 // A single AP is not a multi-link association. Keeping vendor_mlo=1 on the
 // survivor after its peer is deleted must not send the SDK an orphan MLD group.
 if(length(peers)<2)return {enabled:false,group:0};
 for(let peer in peers){
  let peerdev=u.get_all('wireless',peer.band);
  if(!match(peerdev?.htmode??'',/^EHT(20|40|80|160)([+-])?$/))die('All links of an MLD must use EHT');
  if(!match(peer.ap.encryption??'',/^sae(\+ccmp)?$/)||peer.ap.key!=ap.key)
   die('Links of the same MLO SSID require matching WPA3-SAE/CCMP credentials');
 }
 return {enabled:true,group:group_id(u,ap.ssid)};
}
