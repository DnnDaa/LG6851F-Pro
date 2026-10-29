'use strict';
'require view';
'require rpc';
'require ui';
'require dom';
'require lg6851f.theme_v20261015_1 as theme';
const status=rpc.declare({object:'luci.modem5g',method:'get_network',expect:{}});
const connect=rpc.declare({object:'luci.modem5g',method:'ifup',expect:{}});
const disconnect=rpc.declare({object:'luci.modem5g',method:'ifdown',expect:{}});
const radio=rpc.declare({object:'luci.modem5g',method:'set_radio',params:['enabled'],expect:{}});
function parse(data){let link={};try{link=JSON.parse(data.status||'{}');}catch(e){}return {link,settings:data.settings||{}};}
function item(label,value){return E('div',{'class':'net-stat'},[E('span',{'class':'lg-muted'},[label]),E('strong',{},[String(value??'--')])]);}
function elapsed(seconds){seconds=+seconds||0;return`${Math.floor(seconds/3600)} 小时 ${Math.floor(seconds%3600/60)} 分 ${seconds%60} 秒`;}
return view.extend({
 load(){return Promise.resolve();},
 async refresh(manual=false){
  if(this.stopped)return;
  if(this.loading){
   if(manual)this.manualAfterLoad=true;
   return;
  }
  this.loading=true;
  if(manual&&this.refreshButton){
   this.refreshButton.disabled=true;this.refreshButton.textContent='刷新中…';this.refreshButton.classList.add('spinning');this.refreshButton.setAttribute('aria-busy','true');
  }
  try{
   const r=await status();if(r.code)throw Error(r.error||'无法读取 netifd 状态');this.paint(r);
  }catch(e){
   if(!this.stopped)this.note.textContent='状态读取失败：'+e.message;
  }finally{
   this.loading=false;
   if(manual&&this.refreshButton){this.refreshButton.textContent='刷新状态';this.refreshButton.disabled=this.data?.control_active===true;this.refreshButton.classList.remove('spinning');this.refreshButton.setAttribute('aria-busy','false');}
   if(this.manualAfterLoad&&!this.stopped){this.manualAfterLoad=false;setTimeout(()=>this.refresh(true),0);}
  }
 },
 paint(data){
  const {link,settings}=parse(data),up=link.up===true;this.data=data;
  this.title.textContent=up?'蜂窝数据已连接':link.pending?'正在拨号':'蜂窝数据未连接';
  this.title.style.color=up?'var(--lg-positive)':'var(--lg-muted)';
  const gateway=(link.route||[]).find(r=>+r.mask===0)?.nexthop;
  dom.content(this.metrics,[item('数据接口',link.l3_device||link.device),item('连接时长',elapsed(link.uptime)),item('IPv4 地址',(link['ipv4-address']||[]).map(v=>v.address+'/'+v.mask).join(' · ')||'无'),item('网关',gateway||'无')]);
  dom.content(this.config,[item('APN',settings.apn||'自动'),item('IP 类型',({1:'IPv4',2:'IPv6',3:'IPv4 / IPv6'})[settings.iptype]||settings.iptype||'协议默认'),item('认证',({'0':'无','1':'PAP','2':'CHAP','3':'PAP / CHAP'})[settings.auth]||settings.auth||'协议默认'),item('MTU',settings.mtu||'协议默认'),item('拨号协议',link.proto||'ql_datacall'),item('DNS',(link['dns-server']||[]).join(' · ')||'尚未获取')]);
  this.details.textContent=JSON.stringify(link,null,2);
  this.root.querySelectorAll('[data-net-action]').forEach(button=>button.disabled=this.busy||data.control_active===true);
  if(!data.control_active&&this.wasControlActive)this.note.textContent='模组设置已结束，拨号控制已恢复。';this.wasControlActive=!!data.control_active;
  if(data.control_active)this.note.textContent='正在应用模组锁定设置，拨号控制暂时等待。';
 },
 async action(fn,label){
  if(this.busy||this.data?.control_active)return;this.busy=true;this.root.querySelectorAll('[data-net-action]').forEach(b=>b.disabled=true);this.note.textContent=label+'…';
  try{const r=await fn();if(r.code||r.success===false)throw Error(r.error||r.output||'设备未确认请求');this.note.textContent=label+'请求已提交，连接状态自动刷新。';}
  catch(e){this.note.textContent='操作未完成：'+e.message;}
  finally{this.busy=false;await this.refresh();this.root.querySelectorAll('[data-net-action]').forEach(b=>b.disabled=this.data?.control_active===true);}
 },
 render(){
  this.stopped=false;this.busy=false;this.loading=false;this.manualAfterLoad=false;this.title=E('div',{'class':'lg-network'},['正在读取蜂窝状态…']);this.metrics=E('div',{'class':'net-grid'});this.config=E('div',{'class':'net-grid'});this.details=E('pre');this.note=E('div',{'class':'cell-inline-notice',role:'status','aria-live':'polite'});this.refreshButton=E('button',{type:'button','data-net-action':'1',click:()=>this.refresh(true)},['刷新状态']);
  const button=(text,fn,cls)=>E('button',{type:'button','data-net-action':'1','class':cls||'',click:()=>this.action(fn,text)},[text]);
  this.root=E('div',{'class':'lg-control lg-network-page'},[
   E('link',{rel:'stylesheet',href:L.resource('lg6851f/control_v20261015_1.css')}),E('link',{rel:'stylesheet',href:L.resource('lg6851f/cellular_v20261016_1.css')}),
   E('h2',{},['拨号与网络']),E('div',{'class':'lg-hero'},E('div',{style:'width:100%'},[E('span',{'class':'lg-muted'},['当前蜂窝数据连接']),this.title,this.metrics])),
   E('section',{'class':'lg-section'},[E('h3',{},['拨号控制']),E('div',{'class':'lg-row'},[button('连接',connect,'lg-primary'),button('断开',disconnect,'lg-danger'),this.refreshButton]),this.note]),
   E('section',{'class':'lg-section'},[E('h3',{},['网络配置']),this.config,E('div',{'class':'lg-row',style:'margin-top:12px'},[
    E('a',{'class':'net-link',href:L.url('admin/network/network')},['配置 modem5g 网络接口']),E('a',{'class':'net-link',href:L.url('admin/modem5g/overview')},['网络模式、频段与小区锁定'])
   ]),E('p',{'class':'lg-muted'},['APN、认证、IP 类型和 MTU 统一由网络接口配置管理；网络模式和锁定统一在“状态与小区”管理。'])]),
   E('details',{},[E('summary',{'class':'lg-muted'},['蜂窝射频控制']),E('p',{'class':'lg-muted'},['断开拨号只断开数据连接；关闭射频会停止蜂窝注册。关闭前先断开数据连接。']),E('div',{'class':'lg-row'},[
    button('开启蜂窝射频',()=>radio(true)),button('关闭蜂窝射频',async()=>{const down=await disconnect();if(down.code)return down;return radio(false);},'lg-danger')
   ])]),
   E('details',{style:'margin-top:16px'},[E('summary',{'class':'lg-muted'},['连接诊断信息']),this.details])
  ]);
  const poll=async()=>{if(this.stopped||!document.documentElement.contains(this.root))return;if(!document.hidden&&!this.busy)await this.refresh(false);if(!this.stopped)this.timer=setTimeout(poll,5000);};this.timer=setTimeout(poll,0);theme.attach(this.root);return this.root;
 },
 unload(){this.stopped=true;clearTimeout(this.timer);},handleSaveApply:null,handleSave:null,handleReset:null
});
