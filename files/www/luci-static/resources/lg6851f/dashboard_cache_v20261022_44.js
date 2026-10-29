'use strict';
'require baseclass';

// Display-only browser cache. No RPC, hardware query, router file or timer
// survives leaving this view. Stored markup is reconstructed from safe nodes.
var KEY='lg6851f.dashboard.display.v1', LIMIT=262144;
var groups={
 signal:'rat bars op rsrp metric-chips',
 cell:'cellline radio-metrics lteanchor bw ca qci ambr reg neigh',
 traffic:'dl ul rx tx spark-dl spark-ul',
 network:'ip apn sess dns lanip leases wleases carr plmn',
 system:'host uptime dot temps cpu cpu-bar ram ram-bar ram-sub disk disk-bar freqs ppe connections batt batt-l batt-kpi model fwos modem fwmodel fw',
 wifi:'ssid5 ssid24 chan5 chan24 txpower5 txpower24 wenc whid wcountry wcl clist',
 identity:'imei imsi iccid', power:'plug-power'
};
var groupOf=Object.create(null);Object.keys(groups).forEach(function(g){groups[g].split(' ').forEach(function(id){groupOf[id]=g;});});
var tags=/^(DIV|SPAN|B|I|SMALL|BR|TABLE|TBODY|TR|TD|TH|SVG|G|PATH|POLYLINE|LINE|RECT)$/;
var svgTags=/^(SVG|G|PATH|POLYLINE|LINE|RECT)$/;
var attrs=/^(class|title|role|colspan|aria-[a-z-]+|d|points|viewBox|width|height|fill|stroke|stroke-width|x|y|x1|x2|y1|y2)$/;
var styles=['color','background','width','height','display','white-space'];
function serial(node,budget){
 if(--budget.n<0)return null;
 if(node.nodeType===3)return {t:String(node.nodeValue||'').slice(0,8192)};
 if(node.nodeType!==1||!tags.test(node.tagName.toUpperCase()))return null;
 var out={n:node.tagName.toUpperCase(),a:{},s:{},c:[]};
 Array.prototype.forEach.call(node.attributes||[],function(a){if(attrs.test(a.name))out.a[a.name]=a.value.slice(0,2048);});
 // Only the hero's fixed bar identifier is needed inside a cached subtree.
 if(node.id==='fhd-bars')out.a.id='fhd-bars';
 styles.forEach(function(k){var v=node.style.getPropertyValue(k);if(v&&!/url\s*\(|expression|@import/i.test(v))out.s[k]=v;});
 Array.prototype.forEach.call(node.childNodes||[],function(c){var item=serial(c,budget);if(item)out.c.push(item);});
 return out;
}
function material(item,budget){
 if(!item||--budget.n<0)return null;
 if(typeof item.t==='string')return document.createTextNode(item.t.slice(0,8192));
 if(!tags.test(item.n||''))return null;
 var el=svgTags.test(item.n)?document.createElementNS('http://www.w3.org/2000/svg',item.n.toLowerCase()):document.createElement(item.n.toLowerCase());
 Object.keys(item.a||{}).forEach(function(k){var value=String(item.a[k]);if((attrs.test(k)||(k==='id'&&value==='fhd-bars'))&&!/url\s*\(|javascript:/i.test(value))el.setAttribute(k,value.slice(0,2048));});
 styles.forEach(function(k){var v=item.s&&item.s[k];if(typeof v==='string'&&!/url\s*\(|expression|@import/i.test(v))el.style.setProperty(k,v);});
 (Array.isArray(item.c)?item.c:[]).slice(0,1000).forEach(function(child){var n=material(child,budget);if(n)el.appendChild(n);});
 return el;
}
function Cache(root){
 this.root=root;this.items=Object.create(null);this.pending={};this.missing={};this.stamps={};this.clear={};this.scope={};this.signalFallback=false;this.dirty=false;this.lastWrite=0;this.dead=false;
 try{var raw=localStorage.getItem(KEY);if(raw&&raw.length<=LIMIT){var saved=JSON.parse(raw);if(saved&&saved.version===1){
  var scope=saved.scope;
  if(scope&&typeof scope==='object'&&!Array.isArray(scope)){
   ['device','sim','wifiSample'].forEach(function(k){if(typeof scope[k]==='string'||typeof scope[k]==='number')this.scope[k]=scope[k];},this);
   this.scope.temps={};['CPU','蜂窝模组','WIFI温度'].forEach(function(k){var r=scope.temps?.[k];if(r&&Number.isFinite(r.value)&&r.value>=-50&&r.value<=200&&Number.isFinite(r.at)&&r.at>0&&r.at<=Date.now()+60000)this.scope.temps[k]=r;},this);
  }
  Object.keys(saved.items||{}).forEach(function(id){var r=saved.items[id];if(groupOf[id]&&r&&Number.isFinite(r.at)&&r.at>0&&r.at<=Date.now()+60000&&r.tree)this.items[id]=r;},this);
 }}}catch(e){}
 Object.keys(this.items).forEach(function(id){this.restore(id);this.pending[id]=true;},this);
 // One fallback source per entry: keep the restored hero until a current
 // sample replaces it, rather than stepping through another retained cache.
 this.browserSignal=!!this.items.rsrp&&/-?\d+(?:\.\d+)?/.test(this.node('rsrp')?.textContent||'');
 this.holdSignal=this.browserSignal;
}
Cache.prototype.node=function(id){return this.root.querySelector('#fhd-'+id);};
Cache.prototype.restore=function(id){
 var e=this.node(id),r=this.items[id];if(!e||!r)return;
 var tree=material(r.tree,{n:1200});if(!tree)return;
 while(e.firstChild)e.removeChild(e.firstChild);
 while(tree.firstChild)e.appendChild(tree.firstChild);
 if(tree.className)e.className=tree.className;
 if(id==='plug-power')e.hidden=false;
 styles.forEach(function(k){e.style.setProperty(k,tree.style.getPropertyValue(k));});
 delete e._fhdHtml;
};
Cache.prototype.allowed=function(id,value){
 if(!groupOf[id])return true;
 if(this.holdSignal&&['signal','cell'].includes(groupOf[id]))return false;
 if(this.clear[groupOf[id]]){this.missing[id]=false;return true;}
 var plain=value==null?'':String(value).replace(/<[^>]*>/g,'').trim();
 var missing=!plain||/^--(?:\s*\/\s*--)*$/.test(plain);
 if(id==='cellline'&&!this.cell?.nr?.band&&!this.cell?.lte?.band)missing=true;
 if(id==='neigh'&&!this.cell?.current_cells?.length&&!this.cell?.neigh?.some(function(row){return row.serving;}))missing=true;
 if(id==='temps'&&this.info&&!['cpu','modem','wifi'].some(function(k){return this.info.temps?.[k]!=null&&Number.isFinite(Number(this.info.temps[k]));},this))missing=true;
 if(id==='radio-metrics'&&!['nr','lte'].some(function(k){return this.cell?.[k]?.dl_mcs!=null||this.cell?.[k]?.ul_mcs!=null;},this))missing=true;
 if(id==='metric-chips'&&!this.cell?.health?.signal_valid)missing=true;
 if(['lanip','leases','wleases'].includes(id)&&this.info&&!this.info.lan)missing=true;
 if(['ip','dns','apn','sess'].includes(id)&&this.info&&!this.info.wan)missing=true;
 if(id==='modem'&&this.info&&!this.info.modem)missing=true;
 this.missing[id]=missing;
 if(missing&&this.items[id]){this.pending[id]=true;return false;}
 return true;
};
Cache.prototype.remember=function(id){
 if(!groupOf[id]||this.missing[id]||(this.holdSignal&&['signal','cell'].includes(groupOf[id])))return;
 var el=this.node(id);if(!el)return;
 var at=this.stamps[groupOf[id]]||Date.now(),tree=serial(el,{n:1200});if(!tree)return;
 // SIM identifiers remain masked on disk even while the user reveals them.
 if(groupOf[id]==='identity'){
  var t=el.textContent||'';if(t.length>7&&!t.includes('*'))tree.c=[{t:t.slice(0,4)+'****'+t.slice(-3)}];
 }
 this.items[id]={at:at,tree:tree};this.pending[id]=false;this.dirty=true;
 if(!this.saveTimer&&!this.dead)this.saveTimer=setTimeout(function(){this.saveTimer=null;this.flush();}.bind(this),Math.max(1000,15000-(Date.now()-this.lastWrite)));
};
Cache.prototype.context=function(cell,info){
 var now=Date.now();this.cell=cell||null;this.info=info||null;this.clear={};
 if(cell){
  var confirmed=cell.cfun===0||['radio_off','not_registered'].includes(cell.error);
  var sim=cell.ident?.iccid,device=cell.ident?.imei;
  function fingerprint(s){var h=2166136261;for(var n=0;n<s.length;n++)h=Math.imul(h^s.charCodeAt(n),16777619);return String(h>>>0);}
  if(sim){var key=fingerprint(String(sim));if(this.scope.sim&&this.scope.sim!==key)confirmed=true;this.scope.sim=key;}
  if(device){var key=fingerprint(String(device));if(this.scope.device&&this.scope.device!==key){this.items=Object.create(null);this.pending={};this.browserSignal=false;}this.scope.device=key;}
  this.clear.cell=confirmed;this.clear.signal=confirmed;
  // An ordinary current sample may come through the shared backend cache.
  // Only restored/retained/delayed fallback is labelled, never every RPC.
  this.signalFallback=!confirmed&&!!(cell.display_only||cell.health?.retained||cell.health?.delayed||cell.health?.stale||!cell.health?.signal_valid);
  this.holdSignal=this.browserSignal&&this.signalFallback;
  if(confirmed||!this.signalFallback)this.browserSignal=false;
  var age=cell.health?.age;
  this.stamps.signal=Number.isFinite(age)&&age>=0?now-age*1000:now;
  var slow=Number(cell.metadata_ts||cell.ts),server=Number(info?.ts);
  this.stamps.cell=slow>0&&server>=slow?now-(server-slow)*1000:now;
  if(confirmed){['signal','cell'].forEach(function(g){groups[g].split(' ').forEach(function(id){delete this.items[id];delete this.pending[id];},this);},this);}
 }
 if(info){
  var age=Number(info.ts)-Number(info.info_ts||info.ts),at=now-Math.max(0,age||0)*1000;
  this.stamps.system=at;this.stamps.network=at;this.stamps.identity=at;
  this.clear.network=info.wan?.up===0||info.wan?.up===false;
 }
};
Cache.prototype.note=function(ids,label){
 var entries=ids.split(' ').map(function(id){return this.items[id];},this).filter(Boolean);
 if(!entries.length)return '';
 var oldest=Math.min.apply(null,entries.map(function(e){return e.at;}));
 var age=Math.max(0,Math.floor((Date.now()-oldest)/1000));
 var pending=ids.split(' ').some(function(id){return this.pending[id];},this);
 return label+' '+age+' 秒前'+(pending||age>35?' · 更新中':'');
};
Cache.prototype.paintNotes=function(){
 clearTimeout(this.ageTimer);this.ageTimer=null;
 var el=this.node('live-state'),numeric=/-?\d+(?:\.\d+)?/.test(this.node('rsrp')?.textContent||'');
 var fallback=this.signalFallback||this.pending.rsrp||this.pending['metric-chips'];
 // Age remains internal; one updating label covers either fallback source.
 var text=numeric&&fallback?'更新中':'';
 var label=this.node('live-text');if(label)label.textContent=text;
 if(el){el.hidden=!text;el.setAttribute('aria-busy',text?'true':'false');}
 // The text is now constant while pending; the existing snapshot lane
 // updates it. CSS animates the dots without a one-second JS age timer.
};
Cache.prototype.start=function(){this.paintNotes();};
Cache.prototype.flush=function(){
 if(!this.dirty)return;
 try{var text=JSON.stringify({version:1,scope:this.scope||{},items:this.items});if(text.length<=LIMIT)localStorage.setItem(KEY,text);this.dirty=false;this.lastWrite=Date.now();}catch(e){}
};
Cache.prototype.destroy=function(){this.dead=true;clearTimeout(this.ageTimer);clearTimeout(this.saveTimer);this.flush();};

// A cold browser must obtain the device display frame before rendering an
// empty dashboard. Inspect storage only; no RPC, DOM or persistence here.
function hasStoredDisplay(){
 try{
  var raw=localStorage.getItem(KEY);if(!raw||raw.length>LIMIT)return false;
  var saved=JSON.parse(raw);if(saved?.version!==1)return false;
  return Object.keys(saved.items||{}).some(function(id){
   var r=saved.items[id];return !!(groupOf[id]&&r&&Number.isFinite(r.at)&&r.at>0&&r.at<=Date.now()+60000&&r.tree&&tags.test(r.tree.n||'')&&Array.isArray(r.tree.c)&&r.tree.c.length);
  });
 }catch(e){return false;}
}
return baseclass.extend({create:function(root){return new Cache(root);},hasStoredDisplay:hasStoredDisplay});
