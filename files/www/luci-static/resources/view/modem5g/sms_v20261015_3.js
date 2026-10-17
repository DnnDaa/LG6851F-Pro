'use strict';
'require view';
'require rpc';
'require ui';
'require dom';
'require lg6851f.theme_v20261015_1 as theme';
const getPool=rpc.declare({object:'luci.lg6851fcell',method:'sms_get',params:['revision'],expect:{}});
const syncPool=rpc.declare({object:'luci.lg6851fcell',method:'sms_sync',params:['storage','force'],expect:{}});
const submit=rpc.declare({object:'luci.lg6851fcell',method:'sms_send',params:['number','text'],expect:{}});
const job=rpc.declare({object:'luci.lg6851fcell',method:'job_get',params:['id'],expect:{}});
const remove=rpc.declare({object:'luci.lg6851fcell',method:'sms_delete',params:['id'],expect:{}});
const clearPool=rpc.declare({object:'luci.lg6851fcell',method:'sms_clear',expect:{}});
function confirmAction(title,text,action){ui.showModal(title,[E('p',{},[text]),E('div',{'class':'right'},[E('button',{'class':'btn',click:()=>ui.hideModal()},['取消']),' ',E('button',{'class':'btn cbi-button-negative',click:()=>{ui.hideModal();action();}},['确认'])])]);}
return view.extend({
 load(){return Promise.resolve();},
 note(text,error){this.notice.textContent=text||'';this.notice.classList.toggle('cell-error',!!error);},
 async reload(){
  if(this.loading||this.stopped)return;this.loading=true;
  try{const r=await getPool(this.revision||'');if(!r.ok)throw Error(r.error||'短信读取失败');
   this.revision=r.revision;if(!r.not_modified){this.messages=r.messages||[];this.build();}
   if(r.job?.busy&&!this.sending)this.note('正在后台同步短信…');
   else if(r.job?.error&&!this.sending)this.note(r.job.error,true);
  }catch(e){this.note(e.message,true);}finally{this.loading=false;}
 },
 async sync(storage,force){
  if(this.syncing||this.sending)return;this.syncing=true;
  try{const r=await syncPool(storage,force);if(!r.ok)throw Error(r.error||'同步失败');
   this.lastSync=Date.now();this.note(r.cached?'已刷新本地记录，模组同步仍在有效期内。':'正在后台同步，完成后自动更新；不会删除 SIM 短信。');
  }catch(e){this.note(e.message,true);}finally{this.syncing=false;await this.reload();}
 },
 select(number){this.selected=number;this.number.value=number;this.build();},
 build(){
  const groups=new Map();for(const m of this.messages){if(!groups.has(m.number))groups.set(m.number,[]);groups.get(m.number).push(m);}
  const sorted=[...groups].sort((a,b)=>(b[1].at(-1)?.received||0)-(a[1].at(-1)?.received||0));
  this.count.textContent=` · ${this.messages.length} 条 · ${groups.size} 个会话`;
  dom.content(this.conversations,sorted.length?sorted.map(([number,list])=>E('button',{type:'button','class':'sms-conv'+(this.selected===number?' selected':''),click:()=>this.select(number)},[E('b',{},[number]),E('small',{},[list.at(-1)?.text||''])])):E('p',{'class':'cell-note'},['本地池为空。进入页面后会自动同步模组中保存的短信，也可以点击「从 SIM 同步」。']));
  const previous=this.thread.scrollTop,atBottom=this.thread.scrollHeight-this.thread.scrollTop-this.thread.clientHeight<50;
  let list=groups.get(this.selected)||[];const combined=[],multipart=new Map();
  for(const m of list){
   if(!m.ref){combined.push({...m,ids:[m.id]});continue;}
   const key=[m.ref,m.time.slice(0,14),m.direction].join('|');
   if(!multipart.has(key)){const row={...m,parts:[],ids:[]};multipart.set(key,row);combined.push(row);}
   const row=multipart.get(key);row.parts.push(m);row.ids.push(m.id);
  }
  dom.content(this.thread,combined.length?combined.map(m=>{
   const body=m.parts?m.parts.sort((a,b)=>a.part-b.part).map(p=>p.text).join(''):m.text;
   return E('div',{'class':'sms-line '+(m.direction==='out'?'out':'in')},E('div',{'class':'sms-bubble'},[
    E('div',{'class':'sms-meta'},[E('span',{},[m.time||'时间未知',m.status?' · '+m.status:'']),E('button',{type:'button',title:'删除本地消息','aria-label':'删除本地消息',click:()=>confirmAction('删除消息','仅删除设备中的本地记录，SIM 中的短信保留。',()=>this.erase(m.ids))},['×'])]),
    E('div',{},[body]),m.parts&&m.parts.length<m.total?E('small',{'class':'cell-note'},[`长短信待补齐：${m.parts.length}/${m.total}`]):''
   ]));
  }):E('p',{'class':'cell-note'},['选择左侧会话，或输入号码发送短信。']));
  this.thread.scrollTop=atBottom?this.thread.scrollHeight:previous;
 },
 async erase(ids){try{for(const id of ids){const r=await remove(id);if(!r.ok)throw Error(r.error||'删除失败');}this.note('已删除本地记录。');await this.reload();}catch(e){this.note(e.message,true);}},
 async send(){
  if(this.sending)return;const number=this.number.value.trim(),text=this.text.value;
  if(!/^\+?[0-9]{3,20}$/.test(number)){this.note('请输入有效收件号码。',true);return;}
  if(!/^[\x20-\x7e\r\n]{1,160}$/.test(text)){this.note('当前原厂发送接口支持 1–160 个 ASCII 可打印字符；中文接收与显示不受影响。',true);return;}
  this.sending=true;this.sendButton.disabled=true;this.note('正在提交到原厂短信接口，请勿重复发送…');
  try{
   const started=await submit(number,text);if(!started.ok||!started.id)throw Error(started.error||'提交失败');
   this.pendingId=started.id;let finished=null;
   for(let count=0;count<100&&!this.stopped;count++){
    await new Promise(resolve=>{this.jobResolve=resolve;this.jobTimer=setTimeout(resolve,1000);});if(this.stopped)break;
    const r=await job(started.id);if(!r.pending){finished=r;break;}
   }
   if(this.stopped)return;
   if(!finished)throw Error('状态仍待确认，请勿重复发送；稍后刷新查看。');
   if(!finished.ok)throw Error(finished.error||'模组未确认提交，请勿自动重发');
   this.text.value='';this.selected=number;this.note('已提交模组；这不代表收件人已收到。');await this.reload();
  }catch(e){if(!this.stopped)this.note(e.message,true);}finally{this.sending=false;if(!this.stopped)this.sendButton.disabled=false;}
 },
 render(){
  this.stopped=false;this.messages=[];this.lastSync=0;
  this.number=E('input',{type:'tel','class':'sms-number',placeholder:'收件人：号码，如 10086 或 +86…',autocomplete:'off'});
  this.count=E('span',{});this.notice=E('div',{'class':'cell-inline-notice',role:'status','aria-live':'polite'});
  this.conversations=E('div',{'class':'sms-convs'},E('p',{'class':'cell-note'},['正在读取本地短信…']));this.thread=E('div',{'class':'sms-messages'});
  this.text=E('textarea',{rows:1,maxlength:160,placeholder:'短信内容（Enter 发送，Shift+Enter 换行）',keydown:ev=>{if(ev.key==='Enter'&&!ev.shiftKey){ev.preventDefault();this.send();}}});
  this.sendButton=E('button',{type:'button',click:()=>this.send()},['发送']);
  this.root=E('div',{'class':'lg-control lg-cellular'},[
   E('link',{rel:'stylesheet',href:L.resource('lg6851f/control_v20261015_1.css')}),E('link',{rel:'stylesheet',href:L.resource('lg6851f/cellular_v20261015_1.css')}),
   E('h3',{},['短信',this.count]),this.number,
   E('div',{'class':'sms-toolbar'},[
    E('button',{type:'button',click:async()=>{await this.reload();await this.sync('AUTO',false);}},['刷新']),
    E('button',{type:'button',click:()=>this.sync('SM',true)},['从 SIM 同步']),
    E('button',{type:'button','class':'lg-danger',click:()=>confirmAction('清空本地池','删除设备中保存的本地短信，SIM 中的短信保留。再次从 SIM 同步可以重新导入。',async()=>{try{const r=await clearPool();if(!r.ok)throw Error(r.error||'清空失败');this.selected=null;await this.reload();this.note('本地池已清空。');}catch(e){this.note(e.message,true);}})},['清空本地池'])]),
   E('div',{'class':'sms-chat'},[this.conversations,E('div',{'class':'sms-thread'},[this.thread,E('div',{'class':'sms-compose'},[this.text,this.sendButton])])]),
   this.notice,E('p',{'class':'cell-note'},['短信保存在设备本地，重启和保留配置升级后仍可查看。发送沿用原厂 libqlril 接口；气泡右上角可删除本地单条。'])
  ]);
  const refresh=async()=>{if(this.stopped||!document.documentElement.contains(this.root))return;if(!document.hidden){await this.reload();if(Date.now()-this.lastSync>=30000)await this.sync('AUTO',false);}if(!this.stopped)this.timer=setTimeout(refresh,5000);};
  this.timer=setTimeout(refresh,0);theme.attach(this.root);return this.root;
 },
 unload(){this.stopped=true;clearTimeout(this.timer);clearTimeout(this.jobTimer);if(this.jobResolve)this.jobResolve();},handleSaveApply:null,handleSave:null,handleReset:null
});
