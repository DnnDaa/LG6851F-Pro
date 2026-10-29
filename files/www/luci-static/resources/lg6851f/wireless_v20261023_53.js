'use strict';
'require rpc';
'require baseclass';
const callStatus=rpc.declare({object:'lg6851f.wireless',method:'status',expect:{'':{}}});
const KEY='lg6851f.sharedWireless.v3';
let frame=null,flight=null,received=0,listeners=[],failures=0,retryAt=0,savedKey=null,lastError=null;
function usable(value){return value&&value.wifi&&Array.isArray(value.wifi.clients)&&value.wifi.clients_n!=null;}
try{const value=JSON.parse(localStorage.getItem(KEY)||'null');if(usable(value)){frame=value;frame.fresh=false;frame.stale=true;}}catch(e){}
function publish(value){
 if(!value||typeof value!='object')throw new Error('Invalid shared wireless reply');
 if(frame?.wifi?.boot_id&&value.wifi?.boot_id&&frame.wifi.boot_id!==value.wifi.boot_id){frame=null;try{localStorage.removeItem(KEY);}catch(e){}}
 if(usable(value)){
  frame=value;
  const identity=JSON.stringify([value.sample_id,value.wifi.boot_id,value.wifi.clients_n,value.wifi.confirmed_off]);
  if(identity!==savedKey){try{localStorage.setItem(KEY,JSON.stringify(value));savedKey=identity;}catch(e){}}
 }else if(!frame)frame=value;
 received=Date.now();failures=0;retryAt=0;lastError=null;
 listeners.slice().forEach(fn=>{try{fn(frame);}catch(e){}});
 return frame;
}
return baseclass.extend({
 cached(){return frame;},
 // The dashboard transports the management backend's exact shared snapshot.
 // Accepting it neither starts a second RPC nor reads a station table.
 accept(value){return publish(value);},
 read(){
  if(flight)return flight;
  if(Date.now()<retryAt)return frame?Promise.resolve(frame):Promise.reject(lastError);
  if(frame&&Date.now()-received<1000)return Promise.resolve(frame);
  flight=callStatus().then(publish).catch(error=>{
   lastError=error;failures=Math.min(5,failures+1);retryAt=Date.now()+Math.min(30000,1000*Math.pow(2,failures));
   if(frame){frame={...frame,fresh:false,stale:true};return frame;}
   throw error;
  }).finally(()=>{flight=null;});
  return flight;
 },
 subscribe(fn){listeners.push(fn);return ()=>{listeners=listeners.filter(listener=>listener!==fn);};}
});
