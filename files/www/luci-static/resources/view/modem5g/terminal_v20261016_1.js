'use strict';
'require view';
'require rpc';
'require dom';
'require lg6851f.theme_v20261015_1 as theme';
const run=rpc.declare({object:'luci.modem5g',method:'run_at',params:['command'],expect:{}});
const presets=[
 ['基础',['AT','AT+CFUN?','AT+CPIN?','AT+CGMR','AT+ICCID','AT+CIMI','AT+CGSN','AT+CNUM']],
 ['注册 / 信号',['AT+CSQ','AT+CESQ','AT+CEREG?','AT+C5GREG?','AT+COPS?','AT+CGATT?','AT+CGACT?']],
 ['承载',['AT+CGDCONT?','AT+CGPADDR=1','AT+CGCONTRDP','AT+CGEQOSRDP=1','AT+C5GQOSRDP=1']],
 ['工程读取',['AT+ECELL','AT+EINFO?','AT+QNWCFG="nr5g_throughput_info"','AT+QNWCFG="nr5g_center_frequency"','AT+QNWCFG="nr5g_point_a_arfcn"']]
];
return view.extend({
 load(){return Promise.resolve();},
 choose(command){this.input.value=command;this.input.focus();},
 historyPaint(){dom.content(this.history,this.commands.length?this.commands.map(command=>E('button',{type:'button',title:command,click:()=>this.choose(command)},[command])):E('span',{'class':'lg-muted'},['（空）']));},
 append(text){this.transcript=(this.transcript+'\n'+text).slice(-65536);this.output.textContent=this.transcript;this.output.scrollTop=this.output.scrollHeight;},
 async send(){
  if(this.busy||this.stopped)return;const command=this.input.value.trim();
  if(!/^AT[+A-Z0-9?=,._*#;:" ()\/-]*$/i.test(command)||command.length>512){this.append('请输入有效 AT 命令；不支持控制字符和 shell 操作符。');return;}
  this.busy=true;this.sendButton.disabled=true;this.append('> '+command);
  this.commands=[command,...this.commands.filter(v=>v!==command)].slice(0,50);this.cursor=-1;this.historyPaint();
  try{const reply=await run(command);if(this.stopped)return;this.append((reply.output||reply.error||(reply.code?'命令未完成':'（无返回内容）'))+(reply.code?'\n[失败，代码 '+reply.code+']':''));}
  catch(e){if(!this.stopped)this.append('[通信失败] '+e.message);}
  finally{this.busy=false;if(!this.stopped){this.sendButton.disabled=false;this.input.focus();}}
 },
 key(event){
  if(event.key==='Enter'){event.preventDefault();this.send();return;}
  if(event.key!=='ArrowUp'&&event.key!=='ArrowDown')return;
  event.preventDefault();if(!this.commands.length)return;
  if(this.cursor===-1)this.draft=this.input.value;
  this.cursor=Math.max(-1,Math.min(this.commands.length-1,this.cursor+(event.key==='ArrowUp'?1:-1)));
  this.input.value=this.cursor<0?this.draft||'':this.commands[this.cursor];
 },
 render(){
  this.stopped=false;this.commands=[];this.cursor=-1;this.transcript='就绪。';
  this.input=E('input',{type:'text',maxlength:512,spellcheck:'false',autocomplete:'off',placeholder:'AT 命令（↑↓ 翻历史，Enter 发送）',keydown:event=>this.key(event)});
  this.sendButton=E('button',{type:'button',click:()=>this.send()},['发送']);
  this.output=E('pre',{'class':'at-log',role:'log','aria-live':'polite'},['就绪。']);this.history=E('div',{'class':'at-history-list'},E('span',{'class':'lg-muted'},['（空）']));
  this.root=E('div',{'class':'lg-control lg-cellular lg-terminal'},[
   E('link',{rel:'stylesheet',href:L.resource('lg6851f/control_v20261015_1.css')}),E('link',{rel:'stylesheet',href:L.resource('lg6851f/cellular_v20261016_1.css')}),
   E('h3',{},['AT 终端']),E('div',{'class':'at-command'},[this.input,this.sendButton,E('button',{type:'button',click:()=>{this.transcript='就绪。';this.output.textContent=this.transcript;}},['清屏'])]),
   E('div',{'class':'at-workspace'},[this.output,E('aside',{},[E('div',{'class':'lg-muted'},['会话历史（点击复用）']),this.history])]),
   E('div',{'class':'at-presets'},presets.map(([title,commands])=>E('section',{},[E('div',{'class':'lg-muted'},[title]),E('div',{'class':'at-chips'},commands.map(command=>E('button',{type:'button',click:()=>this.choose(command)},[command])))])))
  ]);theme.attach(this.root);return this.root;
 },
 unload(){this.stopped=true;},handleSaveApply:null,handleSave:null,handleReset:null
});
