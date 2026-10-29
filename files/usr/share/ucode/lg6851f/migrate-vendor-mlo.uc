'use strict';
import { cursor } from 'uci';
let u=cursor(), changed=false;
u.foreach('wireless','wifi-iface',function(ap){
 let radio=u.get_all('wireless',ap.device);
 if(radio?.type!='vendor618'||ap.mlo==null)return;
 if(ap.vendor_mlo==null && !u.set('wireless',ap['.name'],'vendor_mlo',ap.mlo))
  die('Cannot preserve vendor MLO setting');
 if(!u.delete('wireless',ap['.name'],'mlo'))die('Cannot remove reserved MLO field');
 changed=true;
});
if(changed && (!u.save('wireless') || !u.commit('wireless')))
 die('Cannot commit vendor MLO migration');
