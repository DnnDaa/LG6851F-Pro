'use strict';
'require view';
'require rpc';
'require ui';
'require fiberhome.common_v20261014_3 as M';

const getStatus=rpc.declare({object:'fiberhomedash',method:'cell_status',expect:{}});
const getControls=rpc.declare({object:'luci.modem5g',method:'get_control_summary',expect:{}});
const getDetail=rpc.declare({object:'luci.modem5g',method:'get_controls',expect:{}});
const setBootRestore=rpc.declare({object:'luci.modem5g',method:'set_boot_restore',params:['enabled'],expect:{}});
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
function cellRows(c){const rows=[];if(c?.nr)rows.push({...c.nr,rat:'nr',serving:true});if(c?.lte)rows.push({...c.lte,arfcn:c.lte.earfcn,rat:'lte',serving:true});for(const r of c?.neigh||[])if(!rows.some(x=>x.rat===r.rat&&x.arfcn===r.arfcn&&x.pci===r.pci))rows.push({...r,serving:false});return rows;}

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
   if(force||!box.children.length){box.replaceChildren(...supported.map(b=>E('label',{'class':'lg-chip'},[E('input',{type:'checkbox',value:b,checked:current.includes(b),change:()=>{this.dirty=true;this.updateBandSummary(rat);}}),E('span',{},prefix+b)])));}
   this.updateBandSummary(rat);
  }
  const r=n.request;
  if(data.profile)this.el('boot-restore').checked=!!data.profile.autoapply;
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

 updateLockDisplay(){
  const r=this.native?.request||{},d=this.controlData||{},c=this.status?.cell||{},box=this.el('locked');if(!box)return;
  let label,detail,tone='';
  if(r.MODE==='cell') {
   const rat=r.RAT||'nr',arfcn=+r.ARFCN,pci=+r.PCI;
   const match=!!c.health?.signal_valid&&cellRows(c).some(x=>x.serving&&x.rat===rat&&+x.arfcn===arfcn&&(pci===65535||+x.pci===pci));
   detail=`${rat==='nr'?'5G NR':'4G LTE'} · ${rat==='nr'?'n':'B'}${r.BAND||'--'} · ARFCN ${r.ARFCN} · PCI ${pci===65535?'任意':r.PCI}`;
   if(d.job?.STATUS==='running'||d.job?.STATUS==='queued'){label='正在操作，等待原厂确认';tone='pending';}
   else if(this.native.current && r.STATUS==='verified' && match){label='本次锁定已确认 · 当前驻留匹配';tone='verified';}
   else if(this.native.current && r.STATUS==='verified'){label='本次提交已确认 · 当前驻留尚未匹配';tone='warning';}
   else if(d.boot_restore?.STATUS==='error'){label='开机恢复未确认';detail+=' · '+(d.boot_restore.DETAIL||'');tone='warning';}
   else {label=match?'保存的小区当前驻留匹配 · 本次锁定尚未确认':'已保存锁定请求 · 本次启动尚未确认';tone='pending';}
  }else {label='未记录已锁定的小区';detail='当前频段与模式以原厂读回为准。';}
  const signature=label+'|'+detail+'|'+tone;
  if(this._lockSignature!==signature){box.className='lg-lock-state '+tone;box.replaceChildren(E('strong',{},label),E('div',{'class':'lg-muted'},detail));this._lockSignature=signature;}
  const restore=this.el('reapply-cell');if(restore)restore.disabled=this.busy||r.MODE!=='cell';
 },
 async saveBootRestore(ev){
  try{const r=await setBootRestore(ev.target.checked);if(!r.success)throw Error('保存失败');this.message(ev.target.checked?'已开启：下次启动等待原厂服务就绪后恢复一次保存的小区锁定。':'已关闭开机恢复；已保存的小区请求仍保留。');}
  catch(e){ev.target.checked=!ev.target.checked;this.message(e.message);}
 },
 reapplySaved(){const r=this.native?.request;if(r?.MODE!=='cell'){this.message('没有可恢复的小区记录。');return;}return this.job('set-cell',{rat:r.RAT,band:+r.BAND,arfcn:+r.ARFCN,pci:r.PCI==='65535'?'':r.PCI});},

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
  const rat=healthy?(c.nr?(c.lte?'5G NSA':'5G SA'):c.lte?'4G LTE':'已注册'):'模组未就绪 / 未注册';
  this.el('network').textContent=rat+' · '+operator;
  const r=healthy?(c.nr||c.lte):null;
  this.el('serving').textContent=r?`${c.nr?'NR':'LTE'} 服务小区 · ${c.nr?'n':'B'}${r.band||'--'} · PCI ${r.pci??'--'} · ARFCN ${r.arfcn??r.earfcn??'--'}${r.bw_mhz?' · '+r.bw_mhz+' MHz':''}`:'暂无新鲜服务小区信息';
  this.el('rsrp').textContent=text(healthy?c.sig?.rsrp:null);
  this.el('metrics').textContent=`RSRQ ${text(healthy?c.sig?.rsrq:null)} · SINR ${text(healthy?c.sig?.sinr:null)}`;
  this.cells=healthy?cellRows(c):[];
  this.updateLockDisplay();
  const body=this.el('cells');body.replaceChildren(...(this.cells.length?this.cells.map(row=>E('tr',{'data-cell':'1',click:()=>this.chooseCell(row)},[`${row.serving?'★ ':''}${row.rat==='nr'?'n':'B'}${row.band||'--'}`,row.pci,row.arfcn,text(row.rsrp),text(row.rsrq),text(row.sinr)].map(v=>E('td',{},String(v))))):[E('tr',{},E('td',{colspan:6},'当前没有新鲜的测量数据'))]));
 },
 render(data){
  this.busy=false;this.dirty=false;
  const section=(title,children)=>E('section',{'class':'lg-section'},[E('h3',{},title),...children]);
  this.root=E('div',{'class':'lg-control'},[
   E('link',{rel:'stylesheet',href:L.resource('lg6851f/control_v20261014_2.css')}),E('h2',{},'状态与小区'),E('style',{},'.lg-lock-state{padding:14px 16px;border:1px solid var(--lg-line);border-radius:10px;background:var(--lg-panel);margin-bottom:14px}.lg-lock-state.verified{border-color:var(--lg-positive)}.lg-lock-state.warning{border-color:var(--lg-danger)}.lg-lock-state.pending{border-style:dashed}.lg-lock-state strong{display:block;margin-bottom:5px}'),
   E('div',{'class':'lg-hero'},[E('div',{},[E('div',{'class':'lg-muted'},'当前驻网'),E('h2',{id:'lg-network','class':'lg-network'},'读取中'),E('div',{id:'lg-serving','class':'lg-muted'},'--')]),E('div',{'class':'lg-signal'},[E('strong',{id:'lg-rsrp'},'--'),' dBm',E('div',{id:'lg-metrics','class':'lg-tag'},'RSRQ -- · SINR --')])]),
   section('网络模式与 EN-DC',[
    E('div',{'class':'lg-row lg-modes'},[['auto','自动'],['lte','仅 4G'],['sa','5G SA'],['nsa','5G NSA']].map(([mode,label])=>E('button',{type:'button','data-mode':mode,click:()=>this.job('set-mode',{mode})},label))),
    E('div',{'class':'lg-row',style:'margin-top:8px'},[E('button',{type:'button',id:'lg-endc',click:()=>this.job('set-endc',{enabled:this.native?.disable!=='NEITHER_IS_DISABLED'})},'允许 EN-DC'),this.button('刷新锁定状态',()=>this.job('get'))]),
    E('p',{'class':'lg-muted'},'模式与频段通过原厂 QLRIL 接口读写。EN-DC 按钮表示允许双连接，实际连接制式由运营商决定。')]),
   section('频段锁定',[
    E('div',{'class':'lg-grid'},['nr','lte'].map(rat=>E('div',{},[E('div',{'class':'lg-band-head'},[E('span',{},rat==='nr'?'NR 频段':'LTE 频段'),E('span',{id:'lg-'+rat+'-summary'},'等待读取')]),E('div',{id:'lg-'+rat+'-bands','class':'lg-chips'}),E('div',{'class':'lg-row'},[this.button('应用 '+rat.toUpperCase()+' 频段',()=>this.applyBands(rat)),this.button('全选',()=>{this.el(rat+'-bands').querySelectorAll('input').forEach(e=>e.checked=true);this.dirty=true;this.updateBandSummary(rat);})])]))),
    E('p',{'class':'lg-muted'},'频段列表来自模组支持掩码；分别应用时保留另一制式的当前频段。锁频与锁小区按照原厂逻辑互斥，应用锁频会解除小区锁定。')]),
   section('邻区与小区锁定',[
    E('div',{id:'lg-locked','class':'lg-lock-state'},'尚未读取锁定记录'),
    E('div',{'class':'lg-row'},[E('label',{},[E('input',{type:'checkbox',id:'lg-boot-restore',change:ev=>this.saveBootRestore(ev)}),' 开机恢复已保存的小区锁定（一次）']),E('button',{type:'button',id:'lg-reapply-cell',click:()=>this.reapplySaved()},'重新应用已保存小区')]),
    E('div',{'class':'lg-row'},[this.button('锁定当前服务小区',()=>{const r=this.cells?.find(c=>c.rat==='nr'&&c.serving)||this.cells?.find(c=>c.serving);if(!r){this.message('当前没有可用服务小区。');return;}this.chooseCell(r);this.message('已填入当前服务小区；请核对频段后点击“应用小区锁定”。');}),this.button('解除小区锁定',()=>this.job('clear-cell'), 'lg-danger')]),
    E('div',{'class':'lg-form'},[
     E('label',{'class':'lg-field'},['制式',E('select',{id:'lg-rat',change:()=>this.updateCellBands()},[E('option',{value:'nr'},'5G NR'),E('option',{value:'lte'},'4G LTE')])]),
     E('label',{'class':'lg-field'},['频段',E('select',{id:'lg-band'})]),
     E('label',{'class':'lg-field'},['ARFCN',E('input',{id:'lg-arfcn',type:'number',min:1,max:3279165,placeholder:'频点编号'})]),
     E('label',{'class':'lg-field'},['PCI（可留空）',E('input',{id:'lg-pci',type:'number',min:0,max:1007,placeholder:'留空只锁频点'})])]),
    this.button('应用小区锁定',()=>this.applyCell(),'lg-primary'),
    E('div',{'class':'lg-table'},E('table',{},[E('thead',{},E('tr',{},['制式 / 频段','PCI','频点','RSRP','RSRQ','SINR'].map(t=>E('th',{},t)))),E('tbody',{id:'lg-cells'})])),
    E('p',{'class':'lg-muted'},'点击表格行可填入小区。应用会短暂中断蜂窝，原厂接口确认后再读回；锁小区时恢复全频段。原厂列表锁定接口不提供完整读回；页面结合本次成功回执和当前驻留确认。跨重启旧记录不会直接显示为已锁定。')]),
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
  requestAnimationFrame(()=>{
   Promise.all([getStatus(),getControls()]).then(r=>{
    this.paintStatus(r[0]);this.paintControls(r[1],true);this.updateCellBands();
    if(['queued','running'].includes(r[1]?.job?.STATUS)){this.message('模组操作进行中，状态会自动更新。');return;}
    if(!r[1].raw || r[1].cache_age==null || r[1].cache_age<0 || r[1].cache_age>300)this.job('get');
    else this.message('已显示共享状态和原厂读回缓存；需要即时重读时点击“刷新锁定状态”。');
   }).catch(e=>this.message('读取失败：'+e.message));
  });
  return this.root;
 },
 unload(){this.disposed=true;clearTimeout(this.timer);},handleSaveApply:null,handleSave:null,handleReset:null
});
