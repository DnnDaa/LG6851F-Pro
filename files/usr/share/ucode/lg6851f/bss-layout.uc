'use strict';
import {cursor} from 'uci';
import {bss_plan} from '/usr/share/ucode/lg6851f/mlo-policy.uc';
let radio=ARGV[0];
if(radio!='radio0' && radio!='radio1')die('Invalid radio');
for(let item in bss_plan(cursor(),radio)) {
 if(ARGV[1]!=null && ARGV[1]!=item.section)continue;
 printf('%d %s %s\n',item.index,item.section,item.ifname);
}
