'use strict';
'require view';
'require rpc';
'require ui';
const getStatus=rpc.declare({object:'luci.lg6851f-led',method:'get_status',expect:{}});
const setMode=rpc.declare({object:'luci.lg6851f-led',method:'set_mode',params:['key','mode','interval','color'],expect:{}});
const labels={auto:'自动',on:'开启',off:'熄灭',blink:'闪烁'},colors={green:'绿色',blue:'蓝色',red:'红色'};
return view.extend({
 load(){return getStatus().catch(()=>({supported:false,leds:[]}));},
 row(key){return this.root.querySelector('[data-led="'+key+'"]');},
 availability(){
  this.root.querySelectorAll('[data-led]').forEach(row=>{const led=(this.data?.leds||[]).find(l=>l.key===row.dataset.led);
   row.querySelectorAll('button,select').forEach(b=>b.disabled=this.busy||!led?.available||(b.classList.contains('led-color')&&!['on','blink'].includes(led?.mode))||(b.classList.contains('led-speed')&&led?.mode!=='blink'));
  });
  this.root.querySelectorAll('.led-bulk button').forEach(b=>b.disabled=this.busy||!(this.data?.leds||[]).some(l=>l.available));
 },
 paint(data){
  this.data=data;
  for(const led of data.leds||[]){const row=this.row(led.key);if(!row)continue;
   row.querySelectorAll('[data-mode]').forEach(b=>{const selected=b.dataset.mode===led.mode;b.classList.toggle('selected',selected);b.setAttribute('aria-pressed',String(selected));});
   const active=led.active_colors||[],color=active.length===1?active[0]:active.length>1?'mixed':'off';
   row.querySelector('.led-state').dataset.color=led.available?color:'unknown';
   row.querySelector('.led-live').textContent=!led.available?led.error||'控制接口未就绪':!led.lit?'当前熄灭':(led.colors||[]).length>1?active.map(c=>colors[c]||c).join(' + ')+(led.blinking?'闪烁':'常亮'):led.blinking?'正在闪烁':'当前亮起';
   row.querySelector('.led-color')?.setAttribute('aria-label',led.label+'颜色');
   const colorSelect=row.querySelector('.led-color');if(colorSelect)colorSelect.value=led.color;
   row.querySelector('.led-speed').value=String(led.interval);
  }
  this.availability();
 },
 async apply(key,mode){
  if(this.busy)return;
  const row=key==='all'?null:this.row(key),led=(this.data?.leds||[]).find(l=>l.key===key);
  const interval=row?+row.querySelector('.led-speed').value:500,color=row?.querySelector('.led-color')?.value||led?.color||'';
  this.busy=true;this.availability();
  try{const r=await setMode(key,mode,interval,color);if(r.status)this.paint(r.status);if(!r.success)throw Error(r.error||'灯光设置未确认');this.root.querySelector('#led-result').textContent=r.unchanged?'设置已应用，无需重复写入配置。':'已保存并应用'+(mode==='auto'?'，网络状态将在下一次更新时显示。':'。');}
  catch(e){this.root.querySelector('#led-result').textContent=e.message;}finally{this.busy=false;this.availability();}
 },
 render(data){
  this.root=E('div',{'class':'lg-control lg-led-page'},[
   E('link',{rel:'stylesheet',href:L.resource('lg6851f/control_v20261014_2.css')}),
   E('style',{},'.lg-led-page{max-width:1120px;margin:auto}.led-intro{margin:0 0 18px}.led-bulk{display:flex;gap:8px;flex-wrap:wrap;margin-bottom:20px}.led-panel{border:1px solid var(--lg-line);border-radius:12px;overflow:hidden;background:var(--lg-panel)}.led-item{display:grid;grid-template-columns:minmax(190px,1fr) minmax(280px,auto);gap:16px;align-items:center;padding:18px 20px;border-bottom:1px solid var(--lg-line)}.led-item:last-child{border-bottom:0}.led-heading{display:flex;align-items:center;gap:12px}.led-title{font-size:15px}.led-position{font-size:11px;color:var(--lg-muted);font-weight:400;margin-left:8px}.led-state{display:inline-block;flex:0 0 12px;height:12px;border-radius:50%;background:var(--lg-line);border:1px solid color-mix(in srgb,var(--lg-text) 12%,transparent)}.led-state[data-color=green]{background:#20a86b}.led-state[data-color=blue]{background:#4085e5}.led-state[data-color=red]{background:#d34751}.led-state[data-color=mixed]{background:linear-gradient(120deg,#d34751,#20a86b,#4085e5)}.led-live{font-size:12px;color:var(--lg-text);margin:4px 0 0 24px}.led-hint{font-size:11px;color:var(--lg-muted);margin:3px 0 0 24px}.led-controls{display:flex;flex-direction:column;align-items:flex-end;gap:9px}.led-buttons{display:flex;gap:6px;flex-wrap:wrap}.lg-led-page .led-buttons button{padding:5px 14px;min-height:32px;font-size:12px}.led-options{display:flex;gap:12px;align-items:center;justify-content:flex-end}.led-options label{display:flex;align-items:center;gap:6px;font-size:11px;color:var(--lg-muted)}.lg-led-page .led-options select{height:28px;width:auto;min-width:86px;padding:3px 7px;font-size:11px}.led-options select:disabled{opacity:.5}.led-note{font-size:12px;color:var(--lg-muted);margin-top:12px}@media(max-width:700px){.led-item{grid-template-columns:1fr;padding:16px;gap:12px}.led-controls{align-items:stretch}.lg-led-page .led-buttons button{flex:1;padding:5px 8px;white-space:nowrap}.led-options{justify-content:flex-start;flex-wrap:wrap}.led-position{margin-left:6px}}'),
   E('h2',{},'灯光控制'),E('p',{'class':'lg-muted led-intro'},'按机身从上到下排列，点击即保存，重启后自动恢复。'),
   E('div',{'class':'led-bulk'},[E('button',{type:'button',click:()=>this.apply('all','off')},'全部熄灭'),E('button',{type:'button',click:()=>this.apply('all','on')},'全部开启'),E('button',{type:'button',click:()=>this.apply('all','auto')},'恢复全部自动')]),
   E('div',{'class':'led-panel'},(data.leds||[]).map((led,i)=>E('section',{'class':'led-item','data-led':led.key},[
    E('div',{},[E('div',{'class':'led-heading'},[E('span',{'class':'led-state','aria-hidden':'true'}),E('strong',{'class':'led-title'},[led.label,E('small',{'class':'led-position'},'灯位 '+(i+1))])]),E('div',{'class':'led-live'}),E('div',{'class':'led-hint'},led.hint)]),
    E('div',{'class':'led-controls'},[
     E('div',{'class':'led-buttons'},Object.entries(labels).map(([mode,label])=>E('button',{type:'button','data-mode':mode,click:()=>this.apply(led.key,mode)},label))),
     E('div',{'class':'led-options'},[
      (led.colors||[]).length>1?E('label',{},['颜色',E('select',{'class':'led-color',change:()=>this.apply(led.key,(this.data.leds||[]).find(l=>l.key===led.key)?.mode||'on')},led.colors.map(color=>E('option',{value:color},colors[color]||color)))]):null,
      E('label',{},['闪烁',E('select',{'class':'led-speed','aria-label':led.label+'闪烁节奏',change:()=>this.apply(led.key,'blink')},[E('option',{value:250},'快 · 0.25 秒'),E('option',{value:500},'标准 · 0.5 秒'),E('option',{value:1000},'慢 · 1 秒')])])])])
   ]))),
   data.supported?null:E('p',{'class':'led-note'},'无法读取灯光控制接口，请稍后刷新。'),
   E('div',{id:'led-result','class':'lg-status',role:'status'},'自动模式跟随设备状态；手动设置独立控制每个灯位。')
  ]);
  this.paint(data);
  const refresh=()=>{if(!document.documentElement.contains(this.root))return;Promise.resolve(document.hidden||this.busy?null:getStatus().then(s=>this.paint(s)).catch(()=>{this.root.querySelector('#led-result').textContent='灯光状态暂时无法读取';})).finally(()=>{this.timer=setTimeout(refresh,10000);});};this.timer=setTimeout(refresh,10000);return this.root;
 },
 unload(){clearTimeout(this.timer);},handleSaveApply:null,handleSave:null,handleReset:null
});
