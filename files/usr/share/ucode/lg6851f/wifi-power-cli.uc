// SPDX-License-Identifier: GPL-2.0-only
'use strict';
import {readfile} from 'fs';
import * as P from './wifi-power-engine.uc';
import * as M from './wifi-power-model.uc';

let result;
switch(ARGV[0]) {
 case 'fingerprint':
  let data=json(readfile('/dev/stdin')??'');
  print(M.fingerprint(data));exit(0);
 case 'status':result=P.status(ARGV[1]);break;
 case 'apply':result=P.apply(ARGV[1],ARGV[2]??'auto');break;
 default:die('Usage: lg6851f-wifi-power status RADIO | apply RADIO [0..30|auto] | fingerprint');
}
print(sprintf('%J\n',result));exit(result.ok?0:1);
