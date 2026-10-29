'use strict';
import {cursor} from 'uci';
import {mlo_group} from '/usr/share/ucode/lg6851f/mlo-policy.uc';
let radio=ARGV[0];if(radio!='radio0'&&radio!='radio1')die('Invalid radio');
printf('%d\n',mlo_group(cursor(),radio,ARGV[1]).group);
