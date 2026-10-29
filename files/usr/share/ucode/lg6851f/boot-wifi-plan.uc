'use strict';
import {cursor} from 'uci';
import {bss_plan} from './mlo-policy.uc';
let u=cursor();
for(let radio in ['radio0','radio1'])for(let b in bss_plan(u,radio))print(b.ifname+'\n');
