 'use strict';
'require baseclass';
function key(r){return r.rat+':'+r.arfcn+':'+r.pci;}
function current(c){
 if(Array.isArray(c?.current_cells))return c.current_cells;
 return (c?.neigh||[]).filter(r=>r.serving===true||r.serving===1).map(r=>({...r,active:true}));
}
function all(c){
 const active=current(c).map(r=>({...r,active:true,serving:r.serving===true||r.serving===1}));
 const seen=new Set(active.map(key));
 for(const r of c?.neigh||[])if(!seen.has(key(r))){seen.add(key(r));active.push({...r,active:false,serving:false,rsrp:c?.health?.signal_valid?r.rsrp:null,rsrq:c?.health?.signal_valid?r.rsrq:null,sinr:c?.health?.signal_valid?r.sinr:null});}
 return active;
}
return baseclass.extend({current:current,all:all});
