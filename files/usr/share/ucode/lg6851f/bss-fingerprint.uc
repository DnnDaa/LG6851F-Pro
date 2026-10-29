// Fingerprint only the BSS being applied. Netifd sends a full radio snapshot;
// using that snapshot for every BSS makes an unrelated SSID edit look changed.
'use strict';
import {readfile} from 'fs';
function canonical(value){
 if(type(value)=='array')return map(value,canonical);
 if(type(value)!='object')return value;
 let out={};for(let key in sort(keys(value)))out[key]=canonical(value[key]);
 return out;
}
let radio=ARGV[0],runtime=ARGV[1],group=int(ARGV[2]??0);
let data=json(readfile('/dev/stdin')??'null');
if(radio!='radio0'&&radio!='radio1')die('Invalid radio');
if(!match(runtime??'',/^[A-Za-z0-9_]+$/))die('Invalid runtime interface');
if(group<0||group>16)die('Invalid MLO group');
let entry=data?.interfaces?.[runtime];
if(type(entry)!='object')die('Missing runtime interface');
let iface={...entry};delete iface.data;
let cfg=data?.config;
if(type(cfg)!='object')die('Missing radio configuration');
// Physical radio changes necessarily affect every BSS. Power tiers are applied
// separately and intentionally do not trigger beacon/authentication rebuilds.
let radio_cfg={band:cfg.band,channel:cfg.channel,country:cfg.country,htmode:cfg.htmode};
print(sprintf('%J',canonical({radio:radio_cfg,interface:iface,mlo_group:group})));
