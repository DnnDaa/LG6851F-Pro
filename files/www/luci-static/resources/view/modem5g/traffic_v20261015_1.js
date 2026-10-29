'use strict';
'require view';
'require rpc';
'require ui';
'require dom';
'require lg6851f.theme_v20261015_1 as theme';
const get=rpc.declare({object:'luci.lg6851fcell',method:'traffic_get',expect:{}});
const set=rpc.declare({object:'luci.lg6851fcell',method:'traffic_set',params:['op','value'],expect:{}});
const GB=1073741824;
function bytes(value){value=Number(value)||0;if(value<1024)return value+' B';if(value<1048576)return(value/1024).toFixed(1)+' KB';if(value<GB)return(value/1048576).toFixed(value<104857600?1:0)+' MB';return(value/GB).toFixed(1)+' GB';}
function gbInput(value){const v=String(value).trim();return /^\d+(\.\d+)?$/.test(v)&&+v<=1000000?String(Math.round(+v*GB)):null;}
function row(key,value){return E('div',{'class':'cell-row'},[E('span',{'class':'cell-key'},[key]),E('span',{'class':'cell-value'},[String(value)])]);}
function card(title,children){return E('section',{'class':'cell-card'},[E('h3',{},[title]),...children]);}
function table(headers,rows){return E('div',{'class':'cell-table-wrap'},E('table',{},[E('thead',{},E('tr',{},headers.map(h=>E('th',{},[h])))),E('tbody',{},rows)]));}
return view.extend({
 load(){return get().catch(e=>({ok:0,error:e.message}));},
 async reload(){if(this.loading||this.stopped)return;this.loading=true;try{const r=await get();if(!r.ok)throw Error(r.error||'统计读取失败');this.paint(r);}catch(e){this.notice.textContent=e.message;this.notice.classList.add('cell-error');}finally{this.loading=false;}},
 async save(button,op,value){
  if(this.busy)return;if(value==null){this.notice.textContent='请输入有效数值。';return;}this.busy=true;button.disabled=true;
  try{const r=await set(op,String(value));if(!r.ok)throw Error(r.error||'保存失败');await this.reload();this.notice.textContent='已保存。';this.notice.classList.remove('cell-error');}
  catch(e){this.notice.textContent=e.message;this.notice.classList.add('cell-error');}finally{this.busy=false;button.disabled=false;}
 },
 paint(st){
  this.data=st;const c=st.cycle,cap=st.cap||{},today=st.today||{rx:0,tx:0};if(!c){this.notice.textContent=st.error||'统计初始化中…';return;}
  const kpi=(big,small)=>E('div',{'class':'cell-kpi'},[E('b',{},[big]),E('span',{},[small])]);
  const alert=cap.level==='over'?(cap.cut_active?'已达上限，移动数据已由流量限制关闭。':'已达本周期上限。'):cap.level==='warn'?'本周期用量已达到提醒阈值。':'';
  dom.content(this.summary,card('流量统计',[
   E('div',{'class':'cell-kpis'},[kpi(bytes(today.rx+today.tx),'今天 · ↓ '+bytes(today.rx)+' ↑ '+bytes(today.tx)),kpi(bytes(c.used),'本计费周期 · '+c.start+' – '+c.last)]),
   row('本周期剩余天数',c.days_left),row('下载',bytes(c.rx)),row('上传',bytes(c.tx)),...(c.adj?[row('本周期校正',(c.adj>0?'+':'−')+bytes(Math.abs(c.adj)))]:[]),
   row('每月上限',cap.bytes?bytes(cap.bytes)+' · '+cap.warn+'% 时提醒':'无'),
   alert?E('p',{'class':'cell-note cell-error',role:'status'},[alert]):'',
   !st.clock_valid?E('p',{'class':'cell-note'},['系统时间尚未同步，当前增量暂存，时间恢复后归入当天。']):'',
   !st.available?E('p',{'class':'cell-note'},['蜂窝数据接口暂不可用，连接恢复后继续统计。']):'',
   E('p',{'class':'cell-note'},['统计实际蜂窝接口 '+(st.device||'ccmni*')+' 的双向流量，包含 VPN 开销，不重复计算 Wi-Fi、LAN 或隧道；与运营商账单可能存在差异。数据保存在内存，每 30 分钟以及正常关机时写盘；突然断电可能丢失尚未保存的最近统计。'])
  ]));
  const days=(st.days||[]).slice(0,31),peak=Math.max(1,...days.map(d=>d.rx+d.tx));
  dom.content(this.daily,card('按天',[table(['日期','','下载','上传','总量'],days.length?days.map(d=>E('tr',{},[
   E('td',{},[d.date]),E('td',{style:'width:38%'},E('div',{'class':'cell-day-bar'},E('i',{style:'width:'+((d.rx+d.tx)/peak*100).toFixed(1)+'%'}))),E('td',{},[bytes(d.rx)]),E('td',{},[bytes(d.tx)]),E('td',{},[bytes(d.rx+d.tx)])
  ])):[E('tr',{},E('td',{colspan:5},['尚无统计，蜂窝产生流量后自动更新。']))]) ]));
  const cycles=(st.cycles||[]).slice(0,24);
  dom.content(this.cycles,card('按计费周期',[table(['计费周期','下载','上传','总量'],cycles.length?cycles.map(v=>E('tr',{},[E('td',{},[v.start+' – '+v.last]),E('td',{},[bytes(v.rx)]),E('td',{},[bytes(v.tx)]),E('td',{},[bytes(v.used)])])):[E('tr',{},E('td',{colspan:4},['尚无周期记录']))])]));
  // Leave edited inputs intact while the summary refreshes.
  if(!this.settingsBuilt){this.buildSettings(st);this.settingsBuilt=true;}
  this.cutButton.textContent=cap.cut_enabled?'开':'关';
 },
 buildSettings(st){
  const s=st.settings||{};
  this.day=E('select',{},Array.from({length:31},(_,i)=>E('option',{value:i+1,selected:+s.cycle_day===i+1},[i===30?'31 · 每月最后一天':String(i+1)])));
  this.day.value=String(s.cycle_day||1);
  this.cap=E('input',{type:'number',min:0,max:1000000,step:'any',value:(s.cap_bytes||0)/GB});
  this.warn=E('input',{type:'number',min:1,max:100,step:1,value:s.warn_percent||90});
  this.used=E('input',{type:'number',min:0,max:1000000,step:'any',placeholder:(st.cycle.used/GB).toFixed(2)});
  const setting=(label,input,op,value,labelButton='保存')=>{const button=E('button',{type:'button',click:()=>this.save(button,op,value())},[labelButton]);return E('div',{'class':'cell-setting'},[E('span',{'class':'cell-key'},[label]),E('div',{'class':'cell-setting-actions'},[input,button])]);};
  this.cutButton=E('button',{type:'button',click:()=>{const next=this.data.cap.cut_enabled?0:1;if(next){ui.showModal('达到上限时关闭移动数据',[E('p',{},['启用后达到流量上限会断开蜂窝数据，可能影响远程管理；下个计费周期自动恢复由此功能关闭的数据连接。']),E('div',{'class':'right'},[E('button',{'class':'btn',click:()=>ui.hideModal()},['取消']),E('button',{'class':'btn cbi-button-positive',click:()=>{ui.hideModal();this.save(this.cutButton,'cut_enabled',1);}},['启用'])])]);}else this.save(this.cutButton,'cut_enabled',0);}},['关']);
  const reset=E('button',{type:'button','class':'lg-danger',click:()=>ui.showModal('清除流量统计',[E('p',{},['清除本地历史和校正值，从当前计数继续；保留上限和计费周期设置。']),E('div',{'class':'right'},[E('button',{'class':'btn',click:()=>ui.hideModal()},['取消']),E('button',{'class':'btn cbi-button-negative',click:()=>{ui.hideModal();this.save(reset,'reset',0);}},['清空'])])])},['清空']);
  dom.content(this.settings,card('计费周期与上限',[
   setting('计费周期开始于每月第几天',this.day,'cycle_day',()=>this.day.value),E('p',{'class':'cell-note'},['31：每月最后一天；短月份自动取该月最后一天。']),
   setting('每月上限（GB，0 表示无）',this.cap,'cap_bytes',()=>gbInput(this.cap.value)),setting('达到上限的此百分比时提醒',this.warn,'warn_percent',()=>this.warn.value),
   E('div',{'class':'cell-setting'},[E('span',{'class':'cell-key'},['达到上限时关闭移动数据']),this.cutButton]),
   setting('本周期已用（GB）',this.used,'used',()=>gbInput(this.used.value),'设置'),E('p',{'class':'cell-note'},['可用运营商显示的当前周期用量校正总量；不改写每天实际测得的上传和下载。1 GB = 1024³ 字节。']),
   E('div',{'class':'cell-setting'},[E('span',{'class':'cell-key'},['清除统计']),reset])
  ]));
 },
 render(st){
  this.stopped=false;this.summary=E('div');this.daily=E('div');this.cycles=E('div');this.settings=E('div');this.notice=E('div',{'class':'cell-inline-notice',role:'status'});
  this.root=E('div',{'class':'lg-control lg-cellular'},[E('link',{rel:'stylesheet',href:L.resource('lg6851f/control_v20261015_1.css')}),E('link',{rel:'stylesheet',href:L.resource('lg6851f/cellular_v20261015_1.css')}),this.summary,this.daily,this.cycles,this.settings,this.notice]);
  if(st.ok)this.paint(st);else this.notice.textContent=st.error||'正在初始化统计…';
  const refresh=async()=>{if(this.stopped||!document.documentElement.contains(this.root))return;if(!document.hidden)await this.reload();if(!this.stopped)this.timer=setTimeout(refresh,30000);};this.timer=setTimeout(refresh,0);theme.attach(this.root);return this.root;
 },
 unload(){this.stopped=true;clearTimeout(this.timer);},handleSaveApply:null,handleSave:null,handleReset:null
});
