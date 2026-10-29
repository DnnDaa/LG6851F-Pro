'use strict';
'require view';
'require lg6851f.theme_v20261015_1 as theme';
'require rpc';
'require ui';
const getStatus=rpc.declare({object:'luci.pwmfan',method:'get_status',expect:{}});
const getSettings=rpc.declare({object:'luci.pwmfan',method:'get_settings',expect:{}});
const saveSettings=rpc.declare({object:'luci.pwmfan',method:'save_settings',params:['mode','percent','apply'],expect:{}});
function fields(raw){const out={};for(const l of (raw||'').split('\n')){const i=l.indexOf('=');if(i>0)out[l.slice(0,i)]=l.slice(i+1);}return out;}
return view.extend({
 load(){return Promise.all([getStatus(),getSettings()]);},
 el(id){return this.root.querySelector('#fan-'+id);},
 syncSelection(){
  let active=null;
  if(this.mode==='auto')active=this.root.querySelector('[data-mode="auto"]');
  else if([0,50,100].includes(this.percent))active=this.root.querySelector(`[data-preset="${this.percent}"]`);
  else active=this.root.querySelector('[data-mode="custom"]');
  this.root.querySelectorAll('[data-mode]').forEach(b=>{const on=b===active;b.classList.toggle('selected',on);b.setAttribute('aria-pressed',on?'true':'false');});
 },
 select(mode,percent){this.mode=mode;if(percent!=null)this.percent=percent;this.el('range').value=this.percent;this.el('range').disabled=mode!=='custom';this.el('percent').textContent=this.percent===0?'安静 · 最低安全风量':this.percent===100?'强劲 · 最高风量':'调节位置 '+this.percent+' / 100';this.syncSelection();},
 paint(status){
  const s=fields(status.controller),temp=status.hottest==null?'--':(status.hottest/1000).toFixed(1);
  this.el('rpm').textContent=status.tach_available?String(status.rpm??s.rpm??'--'):'无转速反馈';
  this.el('temp').textContent=temp;
  this.el('output').textContent=s.fan_type==='3wire'?`档位 ${s.speed_point||'--'}`:`PWM ${s.active==='pwm5'?s.pwm5:s.pwm0??status.pwm??'--'} / 255`;
  const labels={automatic:'自动温控', 'manual-custom':'自定义风速', 'thermal-override':'高温 / 温度读取保护',stalled:'检测到停转，已进入保护',fallback:'安全控制模式'};
  this.el('state').textContent=(labels[s.status]||s.status||'控制器尚未就绪')+` · ${s.fan_type==='4wire'?'四线风扇':s.fan_type==='3wire'?'三线风扇':'型号检测中'} · ${s.active||'--'}`;
  this.el('detail').textContent=status.controller||'尚未读取控制器状态';
  const raw=+(s.active==='pwm5'?s.pwm5:s.pwm0);this.el('meter').style.width=Number.isFinite(raw)?Math.max(0,Math.min(100,raw/255*100))+'%':'0%';
 },
 async save(apply){
  if(this.busy)return;this.busy=true;this.root.querySelectorAll('button').forEach(b=>b.disabled=true);
  try{const r=await saveSettings(this.mode,this.percent,apply);if(!r.success)throw Error(r.error||'保存失败');this.saved={...r.settings};this.el('result').textContent=apply?'设置已保存，控制器将在下一个控制周期应用。':'设置已保存，重启后生效；当前风速未改变。';}
  catch(e){this.el('result').textContent=e.message;}finally{this.busy=false;this.root.querySelectorAll('button').forEach(b=>b.disabled=false);}
 },
 render(data){
  this.disposed=false;
  this.saved={...data[1]};this.mode=this.saved.mode||'auto';this.percent=Number.isFinite(+this.saved.percent)?+this.saved.percent:50;
  const button=(label,fn,cls,preset)=>E('button',{type:'button','class':cls||'','data-mode':preset==null?null:'custom','data-preset':preset,click:ui.createHandlerFn(this,fn)},label);
  this.root=E('div',{'class':'lg-control lg-fan-page'},[
   E('link',{rel:'stylesheet',href:L.resource('lg6851f/control_v20261015_1.css')}),E('h2',{},'风扇控制'),
   E('div',{'class':'lg-stat-grid'},[
    E('div',{'class':'lg-stat'},[E('span',{'class':'lg-muted'},'实际转速'),E('strong',{id:'fan-rpm'},'--'),E('small',{'class':'lg-muted'},'RPM · 来自硬件转速反馈')]),
    E('div',{'class':'lg-stat'},[E('span',{'class':'lg-muted'},'CPU 最高温度'),E('div',{'class':'lg-temp-inline',style:'display:flex;align-items:baseline;gap:.3em;white-space:nowrap'},[E('strong',{id:'fan-temp'},'--'),E('span',{style:'font-size:16px;line-height:1;color:var(--lg-muted);flex:none'},'°C')]),E('small',{'class':'lg-muted'},'温控保护始终生效')]),
    E('div',{'class':'lg-stat'},[E('span',{'class':'lg-muted'},'当前输出'),E('strong',{id:'fan-output'},'--'),E('div',{'class':'lg-meter'},E('i',{id:'fan-meter'}))])]),
   E('section',{'class':'lg-section'},[E('h3',{},'控制方式'),E('div',{'class':'lg-row'},[
    E('button',{type:'button','data-mode':'auto',click:()=>this.select('auto')},'自动温控'),E('button',{type:'button','data-mode':'custom',click:()=>this.select('custom')},'自定义风速'),
    button('安静',()=>this.select('custom',0),'','0'),button('均衡',()=>this.select('custom',50),'','50'),button('强劲',()=>this.select('custom',100),'','100')]),
    E('div',{id:'fan-state','class':'lg-status'},'--')]),
   E('section',{'class':'lg-section'},[E('h3',{},'自定义风量（安全范围）'),E('div',{'class':'lg-row'},['最低风量（保持转动）',E('strong',{id:'fan-percent'}),'最高风量']),
    E('input',{id:'fan-range',type:'range',min:0,max:100,step:1,value:this.percent,input:ev=>{this.percent=+ev.target.value;this.el('percent').textContent=this.percent===0?'安静 · 最低安全风量':this.percent===100?'强劲 · 最高风量':'调节位置 '+this.percent+' / 100';this.syncSelection();}}),
    E('p',{'class':'lg-muted'},'安静档表示最低安全风量，风扇会持续转动，不是停转或 0 RPM。滑块 0–100 是安全范围内的调节位置，不是实际转速百分比。当前四线风扇安静档请求 PWM 115 / 255，最大档为 255 / 255；实际转速请看上方 RPM。三线型号使用原有三档。'),
    E('p',{'class':'lg-muted'},'CPU 达到 80°C、温度读取异常或风扇停转时，保护逻辑可覆盖自定义输出。切换自动 / 自定义不重新探测 PWM 通道。')]),
   E('div',{id:'fan-result','class':'lg-muted',role:'status'},'档位选择后点击“保存并应用”生效；安静档保持散热，不会停转。'),
   E('details',{},[E('summary',{'class':'lg-muted'},'硬件与控制器信息'),E('pre',{id:'fan-detail'})]),
   E('div',{'class':'lg-footer'},[button('保存并应用',()=>this.save(true),'lg-primary'),button('保存',()=>this.save(false)),button('重置',()=>{this.select(this.saved.mode,this.saved.percent);this.el('result').textContent='已恢复最近保存的设置，未改变当前输出。';},'lg-danger')])
  ]);
  this.select(this.mode,this.percent);this.paint(data[0]);
  const root=this.root,alive=()=>!this.disposed&&this.root===root&&document.documentElement.contains(root);
  const refresh=()=>{
   if(!alive())return;
   Promise.resolve().then(()=>document.hidden?null:getStatus()).then(s=>{if(s&&alive())this.paint(s);}).catch(()=>{if(alive()){this.el('rpm').textContent='--';this.el('temp').textContent='--';this.el('state').textContent='状态读取失败';}}).finally(()=>{if(alive())this.timer=setTimeout(refresh,5000);});
  };this.timer=setTimeout(refresh,5000);theme.attach(this.root);return this.root;
 },
 unload(){this.disposed=true;clearTimeout(this.timer);},handleSaveApply:null,handleSave:null,handleReset:null
});
