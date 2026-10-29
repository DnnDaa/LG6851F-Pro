'use strict';
import {cursor} from 'uci';
let u=cursor(),changed=false;
u.foreach('wireless','wifi-device',function(radio){
 if(radio.type!='vendor618')return;
 let name=radio['.name'];
 // The new gain tiers replace the previous absolute power ceiling.
 if(radio.txpower!=null){if(!u.delete('wireless',name,'txpower'))die('Cannot remove legacy power ceiling');changed=true;}
 if(!match(''+(radio.vendor_power_level??''),/^[0123]$/)){
  if(!u.set('wireless',name,'vendor_power_level','1'))die('Cannot migrate gain tier');changed=true;
 }
});
if(changed && (!u.save('wireless') || !u.commit('wireless')))die('Cannot save gain tier migration');
