'use strict';
'require view';
'require lg6851f.theme_v20261015_1 as theme';
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
   E('link',{rel:'stylesheet',href:L.resource('lg6851f/control_v20261015_1.css')}),
   E('style',{},".lg-led-page{width:100%;max-width:1360px;margin-inline:auto;container-type:inline-size;padding:clamp(12px,2.5vw,24px)}\n.lg-led-page .led-intro{margin:0 0 18px;max-width:65ch}\n.lg-led-page .led-bulk{display:grid;grid-template-columns:repeat(auto-fit,minmax(min(100%,9rem),1fr));gap:8px;margin-bottom:18px}\n.lg-led-page .led-panel{display:grid;gap:12px;background:transparent;border:0;overflow:visible}\n.lg-led-page .led-item{display:flex;flex-wrap:wrap;gap:16px 24px;align-items:center;padding:clamp(14px,2.3vw,22px);border:1px solid var(--lg-line);border-radius:12px;background:var(--lg-panel);min-width:0}\n.lg-led-page .led-description{flex:1 1 220px;min-width:0}\n.lg-led-page .led-heading{display:flex;align-items:center;gap:10px;min-width:0}\n.lg-led-page .led-title{display:flex;align-items:center;flex-wrap:wrap;gap:8px;font-size:16px;color:var(--lg-text);line-height:1.4}\n.lg-led-page .led-position{font-size:11px;line-height:1.6;color:var(--lg-muted);font-weight:400;border:1px solid var(--lg-line);padding:0 6px;border-radius:5px;white-space:nowrap}\n.lg-led-page .led-state{display:inline-block;flex:0 0 12px;height:12px;border-radius:50%;background:var(--lg-line);border:1px solid var(--lg-line)}\n.lg-led-page .led-state[data-color=green]{background:#20a86b}.lg-led-page .led-state[data-color=blue]{background:#4085e5}.lg-led-page .led-state[data-color=red]{background:#d34751}.lg-led-page .led-state[data-color=mixed]{background:linear-gradient(120deg,#d34751,#20a86b,#4085e5)}\n.lg-led-page .led-live{font-size:13px;color:var(--lg-text);margin:7px 0 0 22px;overflow-wrap:anywhere}\n.lg-led-page .led-hint{font-size:12px;color:var(--lg-muted);margin:3px 0 0 22px;max-width:55ch;overflow-wrap:anywhere}\n.lg-led-page .led-controls{flex:1 1 310px;min-width:0;display:flex;flex-direction:column;gap:12px}\n.lg-led-page .led-buttons{display:grid;grid-template-columns:repeat(4,minmax(0,1fr));gap:6px;min-width:0}\n.lg-led-page .led-buttons button{width:100%;min-width:0;min-height:42px;padding:8px 4px;font-size:13px;line-height:1.4;white-space:normal;margin:0}\n.lg-led-page .led-options{display:grid;grid-template-columns:repeat(auto-fit,minmax(min(100%,9rem),1fr));gap:8px 12px;min-width:0}\n.lg-led-page .led-options label{display:grid;grid-template-columns:auto minmax(0,1fr);gap:8px;align-items:center;font-size:12px;color:var(--lg-muted);margin:0;min-width:0}\n.lg-led-page .led-options select{height:38px;width:100%;min-width:0;max-width:100%;padding:5px 20px 5px 8px;font-size:12px}\n.lg-led-page .led-note{font-size:12px;color:var(--lg-muted);margin-top:12px}\n.lg-led-page #led-result{margin:18px 0 0}\n@container(max-width:240px){.lg-led-page .led-buttons{grid-template-columns:repeat(2,minmax(0,1fr))}}\n@media(pointer:coarse){.lg-led-page .led-buttons button,.lg-led-page .led-bulk button,.lg-led-page .led-options select{min-height:44px}}\n"),
   E('h2',{},'灯光控制'),E('p',{'class':'lg-muted led-intro'},'按机身从上到下排列，点击即保存，重启后自动恢复。'),
   E('div',{'class':'led-bulk'},[E('button',{type:'button',click:()=>this.apply('all','off')},'全部熄灭'),E('button',{type:'button',click:()=>this.apply('all','on')},'全部开启'),E('button',{type:'button',click:()=>this.apply('all','auto')},'恢复全部自动')]),
   E('div',{'class':'led-panel'},(data.leds||[]).map((led,i)=>E('section',{'class':'led-item','data-led':led.key},[
    E('div',{'class':'led-description'},[E('div',{'class':'led-heading'},[E('span',{'class':'led-state','aria-hidden':'true'}),E('strong',{'class':'led-title'},[led.label,E('small',{'class':'led-position'},'灯位 '+(i+1))])]),E('div',{'class':'led-live'}),E('div',{'class':'led-hint'},led.hint)]),
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
  const refresh=()=>{if(!document.documentElement.contains(this.root))return;Promise.resolve(document.hidden||this.busy?null:getStatus().then(s=>this.paint(s)).catch(()=>{this.root.querySelector('#led-result').textContent='灯光状态暂时无法读取';})).finally(()=>{this.timer=setTimeout(refresh,10000);});};this.timer=setTimeout(refresh,10000);theme.attach(this.root);return this.root;
 },
 unload(){clearTimeout(this.timer);},handleSaveApply:null,handleSave:null,handleReset:null
});
