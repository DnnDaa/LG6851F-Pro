'use strict';
import {readfile} from 'fs';
import {cursor} from 'uci';
let u=cursor(),r=ARGV[0],d=json(readfile('/dev/stdin')),found=false;
if(r!='radio0'&&r!='radio1')die('Invalid radio');
for(let id,v in d.interfaces??{}){
 let c=v?.config;if(c?.mode!='sta'||v.disabled==true||c.disabled==true||v.disabled=='1'||c.disabled=='1')continue;
 if(found)die('One client per radio');found=true;
 if(!match(id,/^[A-Za-z0-9_]+$/))die('Invalid client runtime');
 let n=type(c.network)=='array'?c.network:split(c.network??'',/\s+/);n=filter(n,x=>length(x));
 if(length(n)!=1||n[0]=='lan')die('Choose one independent DHCP uplink');
 let net=u.get_all('network',n[0]);
 if(!net||net.proto!='dhcp'||net.type=='bridge'||length(net.device??'')||length(net.ifname??'')||c.wds==true||c.wds=='1')die('Client requires an unbridged DHCP uplink');
 if(type(c.ssid)!='string'||length(c.ssid)<1||length(c.ssid)>32||match(c.ssid,/[\r\n]/)||match(c.key??'',/[\r\n]/))die('Invalid SSID or key');
 if(!match(c.encryption??'',/^(none|psk|psk2|psk-mixed|sae|sae-mixed)(\+(ccmp|aes|tkip|tkip\+ccmp))?$/))die('Unsupported client security');
 printf('%s\n',id);
}
