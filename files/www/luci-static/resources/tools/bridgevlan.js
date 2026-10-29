'use strict';'require uci';'require network';'require baseclass';const MIN_VLAN_ID=1;const MAX_VLAN_ID=4094;const LUCI_LABEL_SECTION_TYPE='switchvlan';const LUCI_PORT_LABELS_NAME='port_labels';const LUCI_VLAN_LABELS_NAME='vlan_labels';function parseLocal(value){if(value==null||value==='')
return true;switch(String(value).toLowerCase()){case'0':case'false':case'no':case'off':case'disabled':return false;default:return true;}}
function parsePortSpec(spec){const m=String(spec).match(/^([^:]+)(?::([ut*]*))?$/);if(!m)
return null;const flags=m[2]||'';return{port:m[1],tagged:/t/.test(flags),untagged:/u/.test(flags)||flags==='',pvid:/\x2a/.test(flags)||flags===''};}
function formatPortSpec(port,role){if(role==='untagged'||role==='native')
return port;if(role==='tagged')
return'%s:t'.format(port);return null;}
function getBridgeDeviceName(deviceSection){if(!deviceSection)
return null;return deviceSection.name||deviceSection['.name']||null;}
function getVlanSectionPorts(sectionId){return L.toArray(uci.get('network',sectionId,'ports'));}
function isVlanFilteringEnabled(deviceSection){if(!deviceSection)
return false;if(deviceSection.type!=='bridge')
return false;if(deviceSection.vlan_filtering==='1')
return true;const devname=getBridgeDeviceName(deviceSection);let has_vlans=false;uci.sections('network','bridge-vlan',function(bvs){if(bvs.device==devname)
has_vlans=true;});return has_vlans;}
function findActiveBridge(){let match=null;uci.sections('network','device',s=>{if(match||!isVlanFilteringEnabled(s))
return;const ports=collectBridgePorts(s);if(!ports.length)
return;const allSwitchworthy=ports.every(p=>{const t=network.instantiateDevice(p).getType();return t==='switch'||t==='ethernet';});if(allSwitchworthy)
match=s;});return match;}
function collectBridgePorts(deviceSection){const seen={};const devname=getBridgeDeviceName(deviceSection);const section_id=deviceSection?deviceSection['.name']:null;if(!deviceSection)
return[];L.toArray(uci.get('network',section_id,'ports')).forEach(function(port){seen[port]=true;});if(devname){const br=network.instantiateDevice(devname);const brports=(br&&typeof br.getPorts==='function')?br.getPorts():null;if(brports){brports.forEach(function(portdev){const name=portdev?portdev.getName():null;if(name)
seen[name]=true;});}
uci.sections('network','bridge-vlan',function(bvs){if(bvs.device!=devname)
return;getVlanSectionPorts(bvs['.name']).forEach(function(spec){const m=String(spec).match(/^([^:]+)(?::[ut*]+)?$/);if(m)
seen[m[1]]=true;});});}
return Object.keys(seen).sort(L.naturalCompare);}
function collectBridgeVlans(deviceSection){const out=[];if(!deviceSection)
return out;uci.sections('network','bridge-vlan',function(bvs){if(bvs.device!=getBridgeDeviceName(deviceSection))
return;const vid=+bvs.vlan;if(!(vid>=MIN_VLAN_ID&&vid<=MAX_VLAN_ID))
return;out.push({section_id:bvs['.name'],vlan:vid,local:parseLocal(bvs.local),ports:getVlanSectionPorts(bvs['.name']).slice()});});out.sort(function(a,b){return a.vlan-b.vlan;});return out;}
function checkUnsupportedConfig(deviceSection){const issues=[];if(!deviceSection)
return issues;const portUntagged={};uci.sections('network','bridge-vlan',function(bvs){if(bvs.device!=getBridgeDeviceName(deviceSection))
return;const vid=+bvs.vlan;if(!(vid>=MIN_VLAN_ID&&vid<=MAX_VLAN_ID)){issues.push(_('VLAN ID %s is outside the valid range %d–%d.').format(bvs.vlan,MIN_VLAN_ID,MAX_VLAN_ID));return;}
getVlanSectionPorts(bvs['.name']).forEach(function(spec){const parsed=parsePortSpec(spec);if(!parsed){issues.push(_('Port specification "%s" on VLAN %d cannot be parsed.').format(spec,vid));return;}
if(parsed.tagged&&parsed.pvid)
issues.push(_('Port "%s" has the primary VLAN flag together with tagged egress on VLAN %d (PVID ≠ untagged VLAN).').format(parsed.port,vid));else if(parsed.tagged&&parsed.untagged)
issues.push(_('Port "%s" is both tagged and untagged on VLAN %d.').format(parsed.port,vid));else if(parsed.pvid&&!parsed.tagged&&!parsed.untagged)
issues.push(_('Port "%s" is the primary VLAN on VLAN %d but has no untagged egress (PVID ≠ untagged VLAN).').format(parsed.port,vid));if(parsed.untagged){if(portUntagged[parsed.port]!=null&&portUntagged[parsed.port]!==vid){issues.push(_('Port "%s" is configured as untagged on more than one VLAN (%d and %d).').format(parsed.port,portUntagged[parsed.port],vid));}
portUntagged[parsed.port]=vid;}});});return issues;}
function buildPortState(deviceSection,ports){const state={};ports.forEach(function(name){state[name]={name:name,native:null,tagged:[]};});if(!deviceSection)
return state;uci.sections('network','bridge-vlan',function(bvs){if(bvs.device!=getBridgeDeviceName(deviceSection))
return;const vid=+bvs.vlan;getVlanSectionPorts(bvs['.name']).forEach(function(spec){const parsed=parsePortSpec(spec);if(!parsed||!state[parsed.port])
return;if(parsed.untagged)
state[parsed.port].native=vid;else if(parsed.tagged)
state[parsed.port].tagged.push(vid);});});for(let name in state)
state[name].tagged.sort(function(a,b){return a-b;});return state;}
function labelOptionKey(name){return String(name).replace(/[^A-Za-z0-9_]/g,'_');}
function ensureLabelSections(){let port_sid=null,vlan_sid=null;uci.sections('luci',LUCI_LABEL_SECTION_TYPE,function(s){if(s['.name']===LUCI_PORT_LABELS_NAME)
port_sid=s['.name'];else if(s['.name']===LUCI_VLAN_LABELS_NAME)
vlan_sid=s['.name'];});if(!port_sid)
port_sid=uci.add('luci',LUCI_LABEL_SECTION_TYPE,LUCI_PORT_LABELS_NAME);if(!vlan_sid)
vlan_sid=uci.add('luci',LUCI_LABEL_SECTION_TYPE,LUCI_VLAN_LABELS_NAME);return{port:port_sid,vlan:vlan_sid};}
function readPortLabel(portName){const value=uci.get('luci',LUCI_PORT_LABELS_NAME,labelOptionKey(portName));return value!=null?String(value):'';}
function readVlanLabel(vlanId){const value=uci.get('luci',LUCI_VLAN_LABELS_NAME,labelOptionKey(vlanId));return value!=null?String(value):'';}
function writePortLabel(portName,label){ensureLabelSections();const key=labelOptionKey(portName);if(label==null||label==='')
uci.unset('luci',LUCI_PORT_LABELS_NAME,key);else
uci.set('luci',LUCI_PORT_LABELS_NAME,key,label);}
function writeVlanLabel(vlanId,label){ensureLabelSections();const key=labelOptionKey(vlanId);if(label==null||label==='')
uci.unset('luci',LUCI_VLAN_LABELS_NAME,key);else
uci.set('luci',LUCI_VLAN_LABELS_NAME,key,label);}
function vlanPortsEqual(a,b){const norm=list=>L.toArray(list).slice().sort(L.naturalCompare);const sa=norm(a);const sb=norm(b);return sa.length===sb.length&&sa.every((v,i)=>v===sb[i]);}
function getBaseVlanPorts(section_id){if(uci.state.creates.network?.[section_id])
return L.toArray(uci.state.creates.network[section_id].ports);return L.toArray(uci.state.values.network?.[section_id]?.ports);}
function clearPendingVlanPorts(section_id){const c=uci.state.changes.network;const d=uci.state.deletes.network;if(c?.[section_id]){delete c[section_id].ports;if(!Object.keys(c[section_id]).length)
delete c[section_id];}
if(c&&!Object.keys(c).length)
delete uci.state.changes.network;if(d?.[section_id]&&d[section_id]!==true){delete d[section_id].ports;if(!Object.keys(d[section_id]).length)
delete d[section_id];}
if(d&&!Object.keys(d).length)
delete uci.state.deletes.network;}
function setBridgeVlanPorts(section_id,ports){const normalized=L.toArray(ports).slice().sort(L.naturalCompare);const base=getBaseVlanPorts(section_id);if(vlanPortsEqual(normalized,base)){if(uci.state.creates.network?.[section_id]){if(base.length)
uci.set('network',section_id,'ports',base.slice().sort(L.naturalCompare));else
uci.unset('network',section_id,'ports');}
else{clearPendingVlanPorts(section_id);}
return;}
if(normalized.length)
uci.set('network',section_id,'ports',normalized);else
uci.unset('network',section_id,'ports');}
function pruneOrphanLabels(validPorts,validVlans){const portKeys={},vlanKeys={};validPorts.forEach(function(p){portKeys[labelOptionKey(p)]=true;});validVlans.forEach(function(v){vlanKeys[labelOptionKey(v)]=true;});const toRemove=[];uci.sections('luci',LUCI_LABEL_SECTION_TYPE,function(s){const isPortSection=(s['.name']===LUCI_PORT_LABELS_NAME);const isVlanSection=(s['.name']===LUCI_VLAN_LABELS_NAME);if(!isPortSection&&!isVlanSection)
return;const keys=isPortSection?portKeys:vlanKeys;for(let opt in s){if(opt.charAt(0)==='.')
continue;if(!keys[opt])
toRemove.push({section:s['.name'],option:opt});}});toRemove.forEach(function(entry){uci.unset('luci',entry.section,entry.option);});return toRemove.length;}
return baseclass.extend({MIN_VLAN_ID:MIN_VLAN_ID,MAX_VLAN_ID:MAX_VLAN_ID,LUCI_LABEL_SECTION_TYPE:LUCI_LABEL_SECTION_TYPE,LUCI_PORT_LABELS_NAME:LUCI_PORT_LABELS_NAME,LUCI_VLAN_LABELS_NAME:LUCI_VLAN_LABELS_NAME,parsePortSpec:parsePortSpec,formatPortSpec:formatPortSpec,parseLocal:parseLocal,isVlanFilteringEnabled:isVlanFilteringEnabled,findActiveBridge:findActiveBridge,collectBridgePorts:collectBridgePorts,collectBridgeVlans:collectBridgeVlans,checkUnsupportedConfig:checkUnsupportedConfig,buildPortState:buildPortState,labelOptionKey:labelOptionKey,ensureLabelSections:ensureLabelSections,readPortLabel:readPortLabel,readVlanLabel:readVlanLabel,writePortLabel:writePortLabel,writeVlanLabel:writeVlanLabel,pruneOrphanLabels:pruneOrphanLabels,vlanPortsEqual:vlanPortsEqual,setBridgeVlanPorts:setBridgeVlanPorts});