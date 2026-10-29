'use strict';
'require view';
'require rpc';
'require ui';
const getStatus=rpc.declare({object:'luci.lg6851f-led',method:'get_status',expect:{}});
const setMode=rpc.declare({object:'luci.lg6851f-led',method:'set_mode',params:['key','mode','interval'],expect:{}});
const labels={auto:'自动',on:'开启',off:'熄灭',blink:'闪烁'};
return view.extend({
 load(){return getStatus().catch(()=>({leds:[]}));},
 paint(data){
  this.data=data;
  for(const led of data.leds||[]){
   const row=this.root.querySelector('[data-led="'+led.key+'"]');if(!row)continue;
   row.querySelectorAll('[data-mode]').forEach(b=>b.classList.toggle('selected',b.dataset.mode===led.mode));
   row.querySelector('.led-live').textContent=led.trigger==='timer'?'正在闪烁':led.brightness?'当前亮起':'当前熄灭';
   row.querySelector('.led-state').classList.toggle('led-lit',!!led.brightness||led.trigger==='timer');
  }
 },
 async apply(key,mode){
  if(this.busy)return;this.busy=true;this.root.querySelectorAll('button').forEach(b=>b.disabled=true);
  try{const r=await setMode(key,mode,+this.root.querySelector('#led-interval').value);if(!r.success)throw Error(r.error||'灯光设置未确认');this.paint(r.status);this.root.querySelector('#led-result').textContent='已保存并应用'+(mode==='auto'?'，自动状态将在下个控制周期更新。':'。');}
  catch(e){this.root.querySelector('#led-result').textContent=e.message;}finally{this.busy=false;this.root.querySelectorAll('button').forEach(b=>b.disabled=false);}
 },
 render(data){
  this.root=E('div',{'class':'lg-control lg-led-page'},[
   E('link',{rel:'stylesheet',href:L.resource('lg6851f/control_v20261014_2.css')}),
   E('style',{},'.led-item{display:grid;grid-template-columns:minmax(140px,1fr) auto;gap:16px;align-items:center;padding:16px 0;border-bottom:1px solid var(--lg-line)}.led-state{display:inline-block;width:10px;height:10px;border-radius:50%;background:var(--lg-line);margin-right:10px}.led-lit[data-color=green]{background:#20a86b}.led-lit[data-color=amber]{background:#cb8509}.led-lit[data-color=red]{background:#d34751}.led-live{display:block;font-size:12px;color:var(--lg-muted);margin-left:20px}.led-controls{display:flex;gap:6px;flex-wrap:wrap}.led-controls button{padding:6px 14px}@media(max-width:620px){.led-item{grid-template-columns:1fr;gap:10px}.led-controls button{flex:1}}'),
   E('h2',{},'灯光控制'),
   E('p',{'class':'lg-muted'},'直接选择自动、开启、熄灭或闪烁，点击后立即保存。闪烁使用内核定时器，不增加用户态闪灯循环。'),
   E('div',{'class':'lg-row'},[
    E('button',{type:'button',click:()=>this.apply('all','off')},'全部熄灭'),
    E('button',{type:'button',click:()=>this.apply('all','auto')},'恢复全部自动'),
    E('label',{'class':'lg-field'},['闪烁节奏',E('select',{id:'led-interval'},[E('option',{value:250},'快 · 0.25 秒'),E('option',{value:500,selected:true},'标准 · 0.5 秒'),E('option',{value:1000},'慢 · 1 秒')])])]),
   E('div',{},(data.leds||[]).map(led=>E('div',{'class':'led-item','data-led':led.key},[
    E('div',{},[E('span',{'class':'led-state','data-color':led.color}),E('strong',{},led.label),E('span',{'class':'led-live'})]),
    E('div',{'class':'led-controls'},Object.entries(labels).map(([mode,label])=>E('button',{type:'button','data-mode':mode,click:()=>this.apply(led.key,mode)},label)))
   ]))),
   E('div',{id:'led-result','class':'lg-status',role:'status'},'自动模式跟随 Wi-Fi、蜂窝制式与信号状态；手动模式不会再被自动灯光服务覆盖。')
  ]);
  this.paint(data);
  const refresh=()=>{if(!document.documentElement.contains(this.root))return;Promise.resolve(document.hidden||this.busy?null:getStatus().then(s=>this.paint(s)).catch(()=>{})).finally(()=>{this.timer=setTimeout(refresh,10000);});};this.timer=setTimeout(refresh,10000);return this.root;
 },
 unload(){clearTimeout(this.timer);},handleSaveApply:null,handleSave:null,handleReset:null
});
