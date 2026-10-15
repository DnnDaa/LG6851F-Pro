let checks=0;function check(value,label){if(!value)throw Error(label);checks++;}
const specs=[
 ['dashboard','view/fiberhome/home_v20261015_1.js','.fhd-dashboard'],
 ['cellular','view/modem5g/status_cells_v20261015_1.js','.lg-control'],
 ['CPU','view/system/lg6851f-cpufreq-cards_v20261015_1.js','.cpu-page'],
 ['fan','view/pwmfan/control_v20261015_1.js','.lg-control'],
 ['LED','view/system/lg6851f-led_v20261015_1.js','.lg-control']
];
const setup=`
window.fixtureLEDs=[['network','网络','自动跟随有线接口连接',['green']],['cell5g','5G','自动跟随 5G 驻网与信号',['green','blue','red']],['cell4g','4G','自动跟随 4G 驻网与信号',['green','blue','red']],['wifi','Wi-Fi','自动跟随无线接口状态',['green']],['power','电源','自动为常亮；熄灭仅关闭指示灯',['green']]].map(([key,label,hint,colors])=>({key,label,hint,colors,color:'green',mode:'auto',interval:500,available:true,active_colors:key==='wifi'||key==='cell5g'||key==='power'?['green']:[],lit:key==='wifi'||key==='cell5g'||key==='power',blinking:false}));
handlers.get_status=()=>window.testKind==='LED'?{supported:true,leds:fixtureLEDs}:{tach_available:true,rpm:1480,hottest:49300,controller:'fan_type=4wire\\nstatus=automatic\\nactive=pwm0\\npwm0=115\\nrpm=1480'};
handlers.get_settings=()=>({mode:'custom',percent:50});
handlers.save_settings=(mode,percent,apply)=>({success:true,settings:{mode,percent}});
handlers.set_mode=(key,mode,interval,color)=>{fixtureLEDs.forEach(l=>{if(key==='all'||l.key===key){l.mode=mode;l.interval=interval;if(color)l.color=color;l.lit=mode==='on'||mode==='blink'||(mode==='auto'&&l.key==='power');l.active_colors=l.lit?[l.color]:[];l.blinking=mode==='blink';}});return {success:true,status:{supported:true,leds:fixtureLEDs}};};
`;
(async()=>{
 const browser=await chromium.launch({executablePath:'C:/Program Files/Google/Chrome/Application/chrome.exe',headless:true,args:['--disable-gpu']});
 const page=await browser.newPage({viewport:{width:1280,height:960}});const errors=[],results=[];page.on('pageerror',e=>errors.push(e.message));
 await page.route('**/*',route=>{
  const url=new URL(route.request().url());if(url.hostname!=='fixture.local')return route.abort();
  const rel=url.pathname.slice(1);const p=rel.startsWith('theme-assets/')?path.join(root,rel):path.join(res,rel);
  if(fs.existsSync(p)&&fs.statSync(p).isFile())return route.fulfill({body:fs.readFileSync(p),contentType:p.endsWith('.css')?'text/css':'text/plain'});
  return route.fulfill({body:'<!doctype html><html><head><meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1"></head><body><main id="maincontent" style="min-width:0;width:100%;padding:12px;box-sizing:border-box"></main></body></html>',contentType:'text/html'});
 });
 async function frame(){await page.evaluate(()=>new Promise(r=>requestAnimationFrame(()=>requestAnimationFrame(r))));}
 async function open(spec){
  await page.goto('http://fixture.local/preview');await page.addScriptTag({content:prelude+setup+`window.testKind=${JSON.stringify(spec[0])};`});
  await page.evaluate(src=>{window.theme=new Function('baseclass',src)(baseclass);},fs.readFileSync(path.join(res,'lg6851f/theme_v20261015_1.js'),'utf8'));
  await page.evaluate(src=>{window.M=new Function('rpc','baseclass',src)(rpc,baseclass);},fs.readFileSync(path.join(res,'fiberhome/common_v20261015_1.js'),'utf8'));
  await page.evaluate(async src=>{window.screenView=new Function('view','rpc','ui','M','E','L','uci','poll','theme',src)(view,rpc,ui,M,E,L,uci,poll,theme);document.querySelector('#maincontent').append(screenView.render(await screenView.load()));},fs.readFileSync(path.join(res,spec[1]),'utf8'));
  await page.waitForTimeout(80);await frame();
 }
 async function useTheme(name,dark){
  await page.evaluate(async({name,dark})=>{
   document.head.querySelectorAll('[data-fixture-theme]').forEach(e=>e.remove());
   document.documentElement.removeAttribute('style');document.body.removeAttribute('style');
   document.documentElement.dataset.darkmode=String(dark);
   if(name==='generic'){
    // A theme without palette tokens or dark-mode attributes, as with Argon.
    document.documentElement.removeAttribute('data-darkmode');
    document.body.style.cssText=dark?'background:#1b2029;color:#e4e9f0':'background:#f2f4f7;color:#263244';
   }else{
    const files=name==='argon'?(dark?['argon.css','argon-dark.css']:['argon.css']):name==='bootstrap'?['bootstrap.css','bootstrap-mobile.css']:['aurora.css'];
    for(const file of files)await new Promise((resolve,reject)=>{
     const link=document.createElement('link');link.rel='stylesheet';link.dataset.fixtureTheme='1';link.href='http://fixture.local/theme-assets/'+file;link.onload=resolve;link.onerror=reject;document.head.append(link);
    });
   }
  },{name,dark});await frame();
 }
 async function assess(selector){return page.locator(selector).first().evaluate(root=>{
  const canvas=document.createElement('canvas');canvas.width=canvas.height=1;const ctx=canvas.getContext('2d',{willReadFrequently:true});
  function rgba(css){ctx.clearRect(0,0,1,1);ctx.fillStyle=css;ctx.fillRect(0,0,1,1);return Array.from(ctx.getImageData(0,0,1,1).data).map((n,i)=>i===3?n/255:n);}
  function merge(top,bottom){return top.slice(0,3).map((v,i)=>v*top[3]+bottom[i]*(1-top[3]));}
  function background(e){const chain=[];for(let p=e;p;p=p.parentElement)chain.push(p);return chain.reverse().reduce((bg,p)=>merge(rgba(getComputedStyle(p).backgroundColor),bg),[255,255,255]);}
  function lum(c){c=c.map(v=>{v/=255;return v<=.04045?v/12.92:((v+.055)/1.055)**2.4;});return c[0]*.2126+c[1]*.7152+c[2]*.0722;}
  function ratio(a,b){a=lum(a);b=lum(b);return(Math.max(a,b)+.05)/(Math.min(a,b)+.05);}
  const walker=document.createTreeWalker(root,NodeFilter.SHOW_TEXT),failures=[];let count=0;
  for(let node;node=walker.nextNode();){
   if(!node.textContent.trim())continue;const e=node.parentElement;if(e.closest('style,script,option,select,button:disabled,details:not([open]) pre'))continue;
   const rect=e.getBoundingClientRect(),style=getComputedStyle(e);if(!rect.width||!rect.height||style.visibility==='hidden'||style.display==='none')continue;
   const bg=background(e),fg=merge(rgba(style.color),bg),r=ratio(fg,bg);
   const threshold=parseFloat(style.fontSize)>=24||(parseFloat(style.fontSize)>=18.66&&+style.fontWeight>=700)?3:4.5;
   if(r+.03<threshold)failures.push({text:node.textContent.trim().slice(0,35),color:style.color,bg:bg.map(Math.round),ratio:+r.toFixed(2),selector:e.tagName+'.'+e.className});count++;
  }
  return {count,failures,mode:root.dataset.lgTheme,bg:getComputedStyle(root).backgroundColor,overflow:document.documentElement.scrollWidth-document.documentElement.clientWidth,
   globalPollution:['--fhd-text','--lg-text','--surface'].filter(n=>document.documentElement.style.getPropertyValue(n))};
 });}
 for(const spec of specs){
  await open(spec);
  for(const name of ['aurora','bootstrap','argon','generic'])for(const dark of [false,true]){
   await useTheme(name,dark);const r=await assess(spec[2]);results.push({page:spec[0],theme:name,dark,...r});
   check(r.count>8,`${spec[0]} ${name} has measurable text`);
   if(r.failures.length)console.log(await page.locator(spec[2]).evaluate(e=>({rootPanel:e.style.getPropertyValue('--lg-panel'),button:e.querySelector('button')?.outerHTML,computedBg:e.querySelector('button')&&getComputedStyle(e.querySelector('button')).background,computedPanel:e.querySelector('button')&&getComputedStyle(e.querySelector('button')).getPropertyValue('--lg-panel')})));
   check(!r.failures.length,`${spec[0]} ${name} ${dark?'dark':'light'} contrast: `+JSON.stringify(r.failures.slice(0,8)));
   check(r.mode===(dark?'dark':'light'),`${spec[0]} ${name} resolves actual light/dark mode`);
   check(r.globalPollution.length===0,'page palette does not overwrite theme globals');
   if(['argon','bootstrap'].includes(name)&&spec[0]==='LED')await page.screenshot({path:path.join(root,`灯光控制-${name}-${dark?'黑夜':'白天'}.png`),fullPage:true});
  }
  for(const width of [240,280,320,375,600,768,1024,1440,1920]){
   await page.setViewportSize({width,height:960});await frame();const r=await assess(spec[2]);
   check(r.overflow<=1,`${spec[0]} no horizontal page overflow at ${width}: ${r.overflow}`);
   if(spec[0]==='LED'){
    check(await page.locator('.led-item').count()===5,'five physical lights remain visible');
    const clipped=await page.locator('.led-item').evaluateAll(rows=>rows.some(row=>row.scrollWidth>row.clientWidth+1));check(!clipped,'LED actions fit available card width '+width);
   }
  }
  // Large browser text, sidebar-sized parent within a wider screen.
  await page.setViewportSize({width:1280,height:960});
  await page.evaluate(()=>{document.querySelector('#maincontent').style.width='370px';document.documentElement.style.fontSize='20px';});await frame();
  check(await page.locator(spec[2]).evaluate(e=>e.scrollWidth<=e.clientWidth+1),spec[0]+' fits narrow parent with large text');
  if(spec[0]==='LED')await page.screenshot({path:path.join(root,'灯光控制-窄容器大字体.png'),fullPage:true});
  await page.evaluate(()=>{document.querySelector('#maincontent').style.width='100%';document.documentElement.style.fontSize='';});
  if(spec[0]==='dashboard'){
   check(await page.locator('#fhd-qci-label').textContent()==='QCI','dashboard QCI label fixed');
   check(await page.locator('.fhd-sec>h3').allTextContents().then(x=>x.includes('服务小区')),'dashboard service-cell heading');
   await page.evaluate(()=>{for(let n=0;n<120;n++)screenView.update(radio);});check(await page.locator('#fhd-rsrp').textContent()==='-71.1 dBm','signal cache regression remains fixed');
  }
  if(spec[0]==='CPU'){
   await page.getByRole('button',{name:'性能',exact:true}).click();check(await page.evaluate(()=>screenView.mode==='performance'),'CPU profile editing retained');
   check(await page.locator('.cpu-current').textContent()==='1728','CPU register-derived current frequency retained');
  }
  if(spec[0]==='fan'){
   await page.getByRole('button',{name:'安静',exact:true}).click();check(await page.locator('#fan-range').inputValue()==='0'&&(await page.locator('#fan-percent').textContent()).includes('最低安全风量'),'fan quiet safety meaning retained');
   await page.getByRole('button',{name:'保存并应用',exact:true}).click();check(await page.evaluate(()=>calls.some(c=>c.method==='save_settings'&&c.args.join('|')==='custom|0|true')),'fan save RPC retained');
  }
  if(spec[0]==='LED'){
   await page.locator('[data-led="cell5g"] [data-mode="blink"]').click();await page.locator('[data-led="cell5g"] .led-color').selectOption('blue');await page.locator('[data-led="cell5g"] .led-speed').selectOption('250');
   check(await page.locator('[data-led="cell5g"] .led-live').textContent()==='蓝色闪烁','LED color and blink action retained');
   check(await page.locator('[data-led="cell4g"] [data-mode="auto"]').getAttribute('aria-pressed')==='true','LED controls remain independent');
   await page.getByRole('button',{name:'全部熄灭',exact:true}).click();check(await page.locator('[data-mode="off"].selected').count()===5,'all-five off action retained');
   await page.getByRole('button',{name:'恢复全部自动',exact:true}).click();check(await page.locator('[data-mode="auto"].selected').count()===5,'all-five auto action retained');
  }
 }
 check(errors.length===0,'no browser errors: '+errors.join('|'));
 await browser.close();fs.writeFileSync(path.join(root,'theme-browser-check.json'),JSON.stringify({offline_browser_cases:checks,real_theme_variants:6,generic_theme_variants:2,pages:specs.map(s=>s[0]),widths:[240,280,320,375,600,768,1024,1440,1920],results,errors,device_writes:false},null,2));console.log('FIVE_PAGE_THEME_AND_LAYOUT_CHECKS_PASS',checks);
})().catch(e=>{console.error(e);process.exit(1);});
