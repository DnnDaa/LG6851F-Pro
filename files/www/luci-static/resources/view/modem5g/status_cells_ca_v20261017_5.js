'use strict';
'require view';
'require lg6851f.carriers_v20261017_5 as CA';
'require lg6851f.theme_v20261015_1 as theme';
'require rpc';
'require ui';
'require fiberhome.common_v20261016_1 as M';
const refreshSignal=rpc.declare({object:'fiberhomedash',method:'refresh_signal',expect:{}});

const getStatus=rpc.declare({object:'fiberhomedash',method:'cell_status',expect:{}});
const getControls=rpc.declare({object:'luci.modem5g',method:'get_control_summary',expect:{}});
const getDetail=rpc.declare({object:'luci.modem5g',method:'get_controls',expect:{}});
const refreshControls=rpc.declare({object:'luci.modem5g',method:'refresh_controls',expect:{}});
const applyControl=rpc.declare({object:'luci.modem5g',method:'apply_control',params:['action','lte','nr','arfcn','pci','rat','band','mode','enabled'],expect:{}});
function maskBands(hex) {
 if(!hex || !/^(?:[0-9a-f]{8})+$/i.test(hex)) return [];
 const out=[];for(let bit=0;bit<hex.length*4;bit++) if((parseInt(hex.slice(Math.floor(bit/32)*8,Math.floor(bit/32)*8+8),16)>>>bit%32)&1)out.push(bit+1);
 return out;
}
function nativeState(data) {
 const raw=data.raw||'', lte=Array.from(raw.matchAll(/lte band info\s*=\s*([0-9a-f]+)/gi)),nr=Array.from(raw.matchAll(/nr band info\s*=\s*([0-9a-f]+)/gi));
 const nw=raw.match(/The current nwomde:\s*(QL_NW_MODE_[A-Z0-9_]+)/),disable=raw.match(/The current NR Disable Mode:\s*([A-Z_]+)/);
 const values={};for(const line of (data.request||'').split('\n')){const p=line.indexOf('=');if(p>0)values[line.slice(0,p)]=line.slice(p+1).trim();}
 return {lte:lte[0]?maskBands(lte[0][1]):[],nr:nr[0]?maskBands(nr[0][1]):[],supportedLTE:lte[1]?maskBands(lte[1][1]):[],supportedNR:nr[1]?maskBands(nr[1][1]):[],nw:nw?.[1],disable:disable?.[1],request:values,current:values.BOOT_ID===data.boot};
}
function modeOf(s){if(s.nw==='QL_NW_MODE_LTE')return'lte';if(s.disable==='DISABLE_NSA'&&s.nw==='QL_NW_MODE_NR5G')return'sa';if(s.disable==='DISABLE_SA')return'nsa';return s.nw?'auto':'';}
function text(v,suffix){return v==null?'--':String(v)+(suffix||'');}
function cellRows(c){
 const rows=[];
 if(Array.isArray(c?.neigh)&&c.neigh.length){
  for(const r of c.neigh)if(!rows.some(x=>x.rat===r.rat&&+x.arfcn===+r.arfcn&&+x.pci===+r.pci))
   rows.push({...r,serving:r.serving===true||r.serving===1});
 }else{
  if(c?.nr&&c.nr.serving!==false)rows.push({...c.nr,rat:'nr',serving:true});
  if(c?.lte&&c.lte.serving!==false)rows.push({...c.lte,arfcn:c.lte.earfcn,rat:'lte',serving:true});
 }
 return rows;
}

return view.extend({
 load(){return Promise.resolve([{},{}]);},
 el(id){return this.root.querySelector('#lg-'+id);},
 message(value){this.el('result').textContent=value;},
 button(label,fn,kind){return E('button',{type:'button','class':kind||'',click:ui.createHandlerFn(this,fn)},label);},
 setBusy(value){this.busy=value;this.root.querySelectorAll('button,input,select').forEach(el=>{el.disabled=value;});if(!value)this.updateModeButtons();},
 updateModeButtons(){const mode=modeOf(this.native||{});this.root.querySelectorAll('[data-mode]').forEach(b=>b.classList.toggle('selected',b.dataset.mode===mode));const endc=this.el('endc');if(endc){endc.classList.toggle('selected',this.native?.disable==='NEITHER_IS_DISABLED');endc.disabled=this.busy||!['auto','nsa'].includes(mode);endc.title='允许 LTE 与 NR 双连接；实际是否进入 EN-DC 由网络决定。在自动或 NSA 模式下可调整。';}},
 selected(rat){return Array.from(this.el(rat+'-bands').querySelectorAll('input:checked')).map(e=>+e.value);},
 paintControls(data,force){
  this.controlData=data;const n=nativeState(data);this.native=n;
  for(const rat of ['nr','lte']){
   const supported=rat==='nr'?n.supportedNR:n.supportedLTE,current=n[rat],prefix=rat==='nr'?'n':'B';
   const box=this.el(rat+'-bands');
   const signature=supported.join(',')+'|'+current.join(',');
   if(!box.children.length || (force && box._bandSignature!==signature)){box._bandSignature=signature;box.replaceChildren(...supported.map(b=>E('label',{'class':'lg-chip'},[E('input',{type:'checkbox',value:b,checked:current.includes(b),change:()=>{this.dirty=true;this.updateBandSummary(rat);}}),E('span',{},prefix+b)])));}
   this.updateBandSummary(rat);
  }
  const r=n.request;
  this.updateLockDisplay();
  this.el('native-raw').textContent=(data.raw||'尚未读取模组原生状态')+'\n\n'+(data.log||'');
  const previous=this.el('band').value;this.updateCellBands();if(previous)this.el('band').value=previous;
  this.updateModeButtons();
 },
 updateBandSummary(rat){const n=this.native||{},sup=rat==='nr'?n.supportedNR:n.supportedLTE,sel=this.selected(rat);this.el(rat+'-summary').textContent=!sup?.length?'等待原生频段能力':sel.length===sup.length?`自动（支持 ${sup.length} 个）`:`已选 ${sel.length} 个`;},
 async job(action,args){
  if(this.busy||this.reading)return;
  const reading=action==='get';this.reading=reading;
  this.setBusy(!reading);this.message('正在读取模组原生接口…');
  try{
   const r=action==='get'?await refreshControls():await applyControl(action,args?.lte||[],args?.nr||[],args?.arfcn||0,args?.pci??'',args?.rat||'',args?.band||0,args?.mode||'',args?.enabled||false);
   if(r.code||!r.id)throw Error(r.error||'模组未接受操作');
   const deadline=Date.now()+300000;
   for(;;){
    if(this.disposed)return;
    await new Promise(resolve=>setTimeout(resolve,2000));
    const d=await getControls(),j=d.job||{};
    if(j.ID!==r.id)throw Error('模组操作编号已改变，请刷新状态确认');
    if(j.STATUS==='error')throw Error((d.log||'原生接口没有确认此次操作').slice(-1800));
    if(j.STATUS==='done'){this.paintControls(d,true);this.dirty=false;this.message(action==='get'?'已读取原厂频段、网络模式和锁定记录。':'原生接口已确认，当前状态已读回。蜂窝可能需要片刻重新注册。');break;}
    this.message(action==='get'?'正在读取频段与网络模式…':'正在应用并确认原生设置，蜂窝数据会短暂断开；请勿重复点击。');
    if(Date.now()>deadline)throw Error('操作仍未完成，请稍后刷新锁定状态；不会自动重试写入。');
   }
   const status=await getStatus();this.paintStatus(status);
  }catch(e){this.message('未确认完成：'+e.message);}finally{this.reading=false;this.setBusy(false);this.updateLockDisplay();}
 },


 savedCell(){
  const r=this.native?.request||{},p=this.controlData?.profile||{};
  if(r.MODE==='cell')return r;
  // UCI keeps the target even before the first native confirmation after boot.
  if(p.mode==='cell')return {MODE:'cell',RAT:p.rat,BAND:p.band,ARFCN:p.arfcn,PCI:p.pci===''||p.pci==null?'65535':String(p.pci),FROM_PROFILE:true};
  return r;
 },
 updateLockDisplay(){
  const r=this.savedCell(),d=this.controlData||{},c=this.status?.cell||{},box=this.el('locked');if(!box)return;
  let label='未锁定',tone='',values=[];
  if(r.MODE==='cell'){
   const rat=r.RAT||'nr',arfcn=+r.ARFCN,pci=+r.PCI;
   const matched=c.health?.signal_valid?cellRows(c).find(x=>x.serving&&x.rat===rat&&+x.arfcn===arfcn&&(pci===65535||+x.pci===pci)):null;
   if(d.job?.STATUS==='running'||d.job?.STATUS==='queued'||d.boot_restore?.STATUS==='applying'){label='正在应用';tone='pending';}
   else if(!r.FROM_PROFILE && this.native.current && r.STATUS==='verified' && matched){label='已确认 · 驻留匹配';tone='verified';}
   else if(!r.FROM_PROFILE && this.native.current && r.STATUS==='verified'){label='已提交 · 待驻留';tone='warning';}
   else if(d.boot_restore?.STATUS==='error'){label='恢复未确认';tone='warning';}
   else {label='已保存 · 待确认';tone='pending';}
   values=[(rat==='nr'?'NR · n':'LTE · B')+(r.BAND||'--'),pci===65535?'任意':r.PCI??'--',r.ARFCN||'--',text(matched?.rsrp),text(matched?.rsrq),text(matched?.sinr)];
  }
  const signature=JSON.stringify([label,tone,values]);
  if(this._lockSignature!==signature){
   box.className='lg-lock-state '+tone;
   box.replaceChildren(E('div',{'class':'lg-list-heading'},[E('span',{},'锁定小区'),E('span',{'class':'lg-lock-badge'},label)]),this.cellTable(E('tbody',{},E('tr',{},values.length?values.map(v=>E('td',{},String(v))):E('td',{colspan:6},'暂无小区锁定')))));
   this._lockSignature=signature;
  }
  const restore=this.el('reapply-cell');if(restore)restore.disabled=this.busy||r.MODE!=='cell';
 },
 cellTable(body){return E('div',{'class':'lg-table'},E('table',{'class':'lg-cell-table'},[E('colgroup',{},[22,10,20,16,16,16].map(w=>E('col',{style:'width:'+w+'%'}))),E('thead',{},E('tr',{},['制式 / 频段','PCI','频点','RSRP','RSRQ','SINR'].map(t=>E('th',{},t)))),body]));},
 reapplySaved(){const r=this.savedCell();if(r?.MODE!=='cell'){this.message('没有可恢复的小区记录。');return;}return this.job('set-cell',{rat:r.RAT,band:+r.BAND,arfcn:+r.ARFCN,pci:r.PCI==='65535'?'':r.PCI});},

 applyBands(rat){
  const n=this.native;if(!n?.lte.length||!n?.nr.length){this.message('请先刷新锁定状态，读取模组当前频段。');return;}
  const lte=rat==='nr'?n.lte:this.selected('lte'),nr=rat==='lte'?n.nr:this.selected('nr');
  if(!lte.length||!nr.length){this.message('每种制式至少选择一个频段；全选表示自动。');return;}
  return this.job('apply-bands',{lte,nr});
 },
 chooseCell(row){
  this.el('rat').value=row.rat;this.updateCellBands();this.el('arfcn').value=row.arfcn;this.el('pci').value=row.pci;
  if(Array.from(this.el('band').options).some(o=>+o.value===+row.band))this.el('band').value=+row.band;
  this.message(`已选择 ${row.serving?'服务':'邻'}小区：${row.arfcn} / PCI ${row.pci}，请核对频段后应用。`);
 },
 updateCellBands(){const rat=this.el('rat').value,bands=rat==='nr'?this.native?.supportedNR:this.native?.supportedLTE;this.el('band').replaceChildren(E('option',{value:''},'请选择频段'),...(bands||[]).map(b=>E('option',{value:b},(rat==='nr'?'n':'B')+b)));},
 applyCell(){
  const rat=this.el('rat').value,band=+this.el('band').value,arfcn=+this.el('arfcn').value,pci=this.el('pci').value.trim();
  if(!Number.isInteger(arfcn)||arfcn<1||arfcn>(rat==='lte'?262143:3279165)||!band||(pci!==''&&(!/^\d+$/.test(pci)||+pci>(rat==='lte'?503:1007)))){this.message('请检查频段、ARFCN 与 PCI：LTE PCI 0–503，NR PCI 0–1007。');return;}
  return this.job('set-cell',{rat,band,arfcn,pci});
 },
 paintStatus(st){
  this.status=st;const c=st.cell||{},healthy=c.health?.signal_valid;
  const plmn=c.operator?.plmn||'',operator=M.PLMN_CN?.[plmn.slice(0,5)]||c.operator?.name||plmn||'运营商未知';
  const nrServing=c.nr&&c.nr.serving!==false,lteServing=c.lte&&c.lte.serving!==false;
  const rat=healthy?(nrServing?(lteServing?'5G NSA':'5G SA'):lteServing?'4G LTE':'已注册'):'模组未就绪 / 未注册';
  this.el('network').textContent=rat+' · '+operator;
  this.el('rsrp').textContent=text(healthy?c.sig?.rsrp:null);
  this.el('rsrq').textContent=text(healthy?c.sig?.rsrq:null);
  this.el('sinr').textContent=text(healthy?c.sig?.sinr:null);
  this.cells=healthy?cellRows(c):[];
  const known=this.cells.find(x=>x.serving);
  this.el('serving').textContent=known?`${known.rat==='nr'?'NR':'LTE'} 服务小区 · ${known.rat==='nr'?'n':'B'}${known.band||'--'} · PCI ${known.pci??'--'} · ARFCN ${known.arfcn??'--'}${known.bw_mhz?' · '+known.bw_mhz+' MHz':''}`:'暂无新鲜服务小区信息';
  this.el('ca-summary').textContent=CA.description(c.ca);
  const caSignature=JSON.stringify([c.ca?.valid,c.ca?.reason,c.ca?.ts,c.ca?.carriers]);
  if(this._caSignature!==caSignature){this.el('ca-rows').replaceChildren(...CA.rows(c.ca));this._caSignature=caSignature;}
  this.updateLockDisplay();
  const body=this.el('cells'),signature=JSON.stringify(this.cells.map(r=>[r.serving,r.rat,r.band,r.pci,r.arfcn,r.rsrp,r.rsrq,r.sinr]));
  if(this._cellsSignature!==signature){
   for(const [id,serving] of [['cells',true],['neighbors',false]]){
    const items=this.cells.filter(row=>row.serving===serving);
    this.el(id).replaceChildren(...(items.length?items.map(row=>E('tr',{'data-cell':'1',click:()=>this.chooseCell(row)},[`${row.rat==='nr'?'NR · n':'LTE · B'}${row.band||'--'}`,row.pci,row.arfcn,text(row.rsrp),text(row.rsrq),text(row.sinr)].map(v=>E('td',{},String(v))))):[E('tr',{},E('td',{colspan:6},serving?'当前没有服务小区测量数据':'当前没有邻区测量数据'))]));
   }
   this._cellsSignature=signature;
  }

 },
 render(data){
  this.busy=false;this.dirty=false;
  const section=(title,children)=>E('section',{'class':'lg-section'},[E('h3',{},title),...children]);
  this.root=E('div',{'class':'lg-control'},[
   E('link',{rel:'stylesheet',href:L.resource('lg6851f/control_v20261015_1.css')}),E('h2',{},'状态与小区'),E('style',{},"\n.lg-control .lg-hero>div:first-child{min-width:0}.lg-control #lg-serving{overflow-wrap:anywhere}\n.lg-control .lg-signal{display:flex;flex-direction:column;align-items:flex-end;gap:6px;flex:0 0 auto}\n.lg-control .lg-signal-value{display:flex;align-items:baseline;justify-content:flex-end;gap:5px;font-variant-numeric:tabular-nums}\n.lg-control .lg-signal-value strong{line-height:1.05}.lg-control .lg-signal-value small{font-size:12px;color:var(--lg-muted)}\n.lg-control .lg-signal-metrics{display:flex;align-items:center;gap:8px}\n.lg-control .lg-signal-metrics>span{display:flex;align-items:baseline;gap:5px;border:1px solid var(--lg-line);border-radius:6px;padding:2px 7px;background:var(--lg-panel);font-size:12px;line-height:1.4;color:var(--lg-muted)}\n.lg-control .lg-signal-metrics b{font-size:12px;font-weight:600;color:var(--lg-text);font-variant-numeric:tabular-nums}\n.lg-lock-state{margin-bottom:14px}.lg-control .lg-list-heading{display:flex;align-items:center;justify-content:space-between;gap:12px;font-size:12px;color:var(--lg-muted)}\n.lg-control .lg-lock-badge{font-size:11px;border:1px solid var(--lg-line);border-radius:5px;padding:1px 7px;line-height:1.7}\n.lg-control .verified .lg-lock-badge{color:var(--lg-positive);border-color:currentColor}.lg-control .warning .lg-lock-badge{color:var(--lg-danger);border-color:currentColor}.lg-control .pending .lg-lock-badge{border-style:dashed}\n.lg-control .lg-cell-table{table-layout:fixed;min-width:520px}.lg-control .lg-cell-table th,.lg-control .lg-cell-table td{font-variant-numeric:tabular-nums;text-align:right;white-space:nowrap}\n.lg-control .lg-cell-table th:first-child,.lg-control .lg-cell-table td:first-child{text-align:left}\n@media(max-width:700px){.lg-control .lg-hero{flex-direction:column;gap:14px}.lg-control .lg-signal{align-items:flex-start}.lg-control .lg-signal-value{justify-content:flex-start}.lg-control .lg-list-heading{flex-wrap:wrap}}\n"),
   E('div',{'class':'lg-hero'},[E('div',{},[E('div',{'class':'lg-muted'},'当前驻网'),E('h2',{id:'lg-network','class':'lg-network'},'读取中'),E('div',{id:'lg-serving','class':'lg-muted'},'--')]),E('div',{'class':'lg-signal'},[E('div',{'class':'lg-signal-value'},[E('strong',{id:'lg-rsrp'},'--'),E('small',{},'dBm')]),E('div',{'class':'lg-signal-metrics'},[E('span',{},['RSRQ',E('b',{id:'lg-rsrq'},'--')]),E('span',{},['SINR',E('b',{id:'lg-sinr'},'--')])])])]),
   section('载波聚合',[E('div',{id:'lg-ca-summary','class':'lg-muted',style:'margin-bottom:10px'},'读取中······'),CA.table()]),
   section('网络模式与 EN-DC',[
    E('div',{'class':'lg-row lg-modes'},[['auto','自动'],['lte','仅 4G'],['sa','5G SA'],['nsa','5G NSA']].map(([mode,label])=>E('button',{type:'button','data-mode':mode,click:()=>this.job('set-mode',{mode})},label))),
    E('div',{'class':'lg-row',style:'margin-top:8px'},[E('button',{type:'button',id:'lg-endc',click:()=>this.job('set-endc',{enabled:this.native?.disable!=='NEITHER_IS_DISABLED'})},'允许 EN-DC'),this.button('刷新锁定状态',()=>this.job('get'))]),
    E('p',{'class':'lg-muted'},'模式与频段通过原厂 QLRIL 接口读写。EN-DC 按钮表示允许双连接，实际连接制式由运营商决定。')]),
   section('频段锁定',[
    E('div',{'class':'lg-grid'},['nr','lte'].map(rat=>E('div',{},[E('div',{'class':'lg-band-head'},[E('span',{},rat==='nr'?'NR 频段':'LTE 频段'),E('span',{id:'lg-'+rat+'-summary'},'等待读取')]),E('div',{id:'lg-'+rat+'-bands','class':'lg-chips'}),E('div',{'class':'lg-row'},[this.button('应用 '+rat.toUpperCase()+' 频段',()=>this.applyBands(rat)),this.button('全选',()=>{this.el(rat+'-bands').querySelectorAll('input').forEach(e=>e.checked=true);this.dirty=true;this.updateBandSummary(rat);})])]))),
    E('p',{'class':'lg-muted'},'频段列表来自模组支持掩码；分别应用时保留另一制式的当前频段。锁频与锁小区按照原厂逻辑互斥，应用锁频会解除小区锁定。')]),
   section('邻区与小区锁定',[
    E('div',{id:'lg-locked','class':'lg-lock-state'},'尚未读取锁定记录'),
    E('div',{'class':'lg-row'},[this.button('填入当前服务小区',()=>{const r=this.cells?.find(c=>c.rat==='nr'&&c.serving)||this.cells?.find(c=>c.serving);if(!r){this.message('当前没有可用服务小区。');return;}this.chooseCell(r);this.message('已填入当前服务小区；请核对频段后点击“应用小区锁定”。');}),this.button('解除小区锁定',()=>this.job('clear-cell'), 'lg-danger'),E('button',{type:'button',id:'lg-reapply-cell',click:()=>this.reapplySaved()},'重新应用')]),
    E('div',{'class':'lg-form'},[
     E('label',{'class':'lg-field'},['制式',E('select',{id:'lg-rat',change:()=>this.updateCellBands()},[E('option',{value:'nr'},'5G NR'),E('option',{value:'lte'},'4G LTE')])]),
     E('label',{'class':'lg-field'},['频段',E('select',{id:'lg-band'})]),
     E('label',{'class':'lg-field'},['ARFCN',E('input',{id:'lg-arfcn',type:'number',min:1,max:3279165,placeholder:'频点编号'})]),
     E('label',{'class':'lg-field'},['PCI（可留空）',E('input',{id:'lg-pci',type:'number',min:0,max:1007,placeholder:'留空只锁频点'})])]),
    this.button('应用小区锁定',()=>this.applyCell(),'lg-primary'),
    E('h3',{},'当前小区'),this.cellTable(E('tbody',{id:'lg-cells'})),E('h3',{style:'margin-top:16px'},'邻区测量'),this.cellTable(E('tbody',{id:'lg-neighbors'}))]),
   section('恢复自动',[
    E('div',{'class':'lg-row'},[this.button('频段恢复自动',()=>this.job('reset-bands')),this.button('全部重置为自动',()=>this.job('reset-all'),'lg-danger')]),
    E('p',{'class':'lg-muted'},'全部重置会解除小区锁定、恢复支持的全部频段，并将网络模式和 EN-DC 恢复为自动。')]),
   E('div',{id:'lg-result','class':'lg-status',role:'status'},'读取状态中…'),
   E('details',{toggle:ev=>{if(ev.target.open)getDetail().then(d=>{this.el('native-raw').textContent=(d.raw||'')+'\n\n'+(d.log||'');});}},[E('summary',{'class':'lg-muted'},'原生读回与操作记录'),E('pre',{id:'lg-native-raw'})])
  ]);
  this.paintControls(data[1],true);this.updateCellBands();this.paintStatus(data[0]);
  const refresh=()=>{
   if(!document.documentElement.contains(this.root))return;
   Promise.resolve(document.hidden||this.busy?null:Promise.all([getStatus(),getControls()]).then(r=>{this.paintStatus(r[0]);this.paintControls(r[1],!this.dirty);}).catch(()=>this.paintStatus({}))).finally(()=>{this.timer=setTimeout(refresh,5000);});
  };
  this.timer=setTimeout(refresh,5000);
  const refreshRadio=()=>{
   if(this.disposed||!document.documentElement.contains(this.root))return;
   Promise.resolve(document.hidden||this.busy?null:refreshSignal()).catch(()=>{}).finally(()=>{
    if(!this.disposed)this.signalTimer=setTimeout(refreshRadio,25000);
   });
  };
  this.signalTimer=setTimeout(refreshRadio,0);
  requestAnimationFrame(()=>{
   Promise.all([getStatus(),getControls()]).then(r=>{
    this.paintStatus(r[0]);this.paintControls(r[1],true);this.updateCellBands();
    if(['queued','running'].includes(r[1]?.job?.STATUS)){this.message('模组操作进行中，状态会自动更新。');return;}
    if(!r[1].raw || r[1].cache_age==null || r[1].cache_age<0 || r[1].cache_age>300)this.job('get');
    else this.message('已显示共享状态和原厂读回缓存；需要即时重读时点击“刷新锁定状态”。');
   }).catch(e=>this.message('读取失败：'+e.message));
  });
  theme.attach(this.root);return this.root;
 },
 unload(){this.disposed=true;clearTimeout(this.timer);clearTimeout(this.signalTimer);},handleSaveApply:null,handleSave:null,handleReset:null
});
