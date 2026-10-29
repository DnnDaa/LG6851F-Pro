'use strict';
'require baseclass';

/* Resolve the installed theme once on entry and on theme changes. The palette
 * belongs to the page, never to html/body; unrelated LuCI views are untouched. */
const roots = new Map();
let pending = false, watching = false;
const mix = (a,b,t) => a.map((v,i) => Math.round(v*(1-t)+b[i]*t));
const rgb = a => 'rgb('+a.join(',')+')';
function luminance(c) {
 const s=c.map(v=>{v/=255;return v<=.04045?v/12.92:Math.pow((v+.055)/1.055,2.4);});
 return s[0]*.2126+s[1]*.7152+s[2]*.0722;
}
function contrast(a,b) {const x=luminance(a),y=luminance(b);return (Math.max(x,y)+.05)/(Math.min(x,y)+.05);}
function readable(color, surfaces, fallback) {
 if(surfaces.every(s=>contrast(color,s)>=4.6))return color;
 for(let n=1;n<=20;n++){const c=mix(color,fallback,n/20);if(surfaces.every(s=>contrast(c,s)>=4.6))return c;}
 return fallback;
}
function paint(root) {
 const host=root.parentElement||document.body, styles=getComputedStyle(host);
 const probe=document.createElement('span');
 probe.setAttribute('aria-hidden','true');probe.style.cssText='position:absolute;visibility:hidden;pointer-events:none;width:0;height:0;overflow:hidden';
 host.appendChild(probe);
 const canvas=document.createElement('canvas');canvas.width=canvas.height=1;
 const ctx=canvas.getContext('2d',{willReadFrequently:true});
 function color(value) {
  if(!value||!CSS.supports('color',value))return null;
  probe.style.color=value;ctx.clearRect(0,0,1,1);ctx.fillStyle=getComputedStyle(probe).color;ctx.fillRect(0,0,1,1);
  const c=ctx.getImageData(0,0,1,1).data;return c[3]>=250?Array.from(c).slice(0,3):null;
 }
 function token(names) {for(const n of names){const c=color(styles.getPropertyValue(n).trim());if(c)return c;}return null;}
 let actual=null;
 for(let e=host;e&&!actual;e=e.parentElement)actual=color(getComputedStyle(e).backgroundColor);
 const hintedDark=!!document.querySelector('html[data-darkmode="true"],body[data-darkmode="true"],html[data-theme="dark"],body[data-theme="dark"],html.dark,body.dark');
 const high=token(['--surface','--background-color-high']);
 let panel=high||actual||(hintedDark?[23,28,37]:[255,255,255]);
 const dark=luminance(panel)<.35;
 const fallback=dark?[235,240,247]:[26,36,49];
 let low=token(['--surface-sunken','--background-color-low']);
 if(!low||(luminance(low)<.35)!==dark)low=mix(panel,fallback,.035);
 const surfaces=[panel,low];
 const candidate=token(['--text','--text-color-high'])||color(styles.color)||fallback;
 const text=surfaces.every(s=>contrast(candidate,s)>=4.6)?candidate:fallback;
 const muted=readable(token(['--text-muted','--text-color-medium'])||mix(panel,text,.72),surfaces,text);
 const accent=readable(token(['--brand','--primary-color-high','--primary','--dark-primary'])||(dark?[66,205,189]:[0,120,112]),surfaces,text);
 const soft=mix(panel,accent,.055);surfaces.push(soft);
 const positive=readable(token(['--success','--success-color-high'])||(dark?[84,209,156]:[20,120,77]),surfaces,text);
 const warning=readable(token(['--warning','--warn-color-high'])||(dark?[255,198,91]:[143,90,0]),surfaces,text);
 const danger=readable(token(['--danger','--error-color-high'])||(dark?[255,139,151]:[180,42,62]),surfaces,text);
 const onAccent=contrast(accent,[255,255,255])>=4.6?[255,255,255]:[15,22,31];
 const line=mix(panel,text,.32),hover=mix(panel,text,.06);
 const fhd={surface:panel,bg:low,text:readable(text,surfaces,fallback),muted:readable(muted,surfaces,text),subtle:readable(muted,surfaces,text),line,brand:accent,positive,warning,danger,info:accent,'on-brand':onAccent,hero:soft,hover};
 const lg={panel,bg:low,text:fhd.text,muted:fhd.muted,line,accent,positive,danger,'on-accent':onAccent,soft};
 for(const [prefix,values] of [['fhd',fhd],['lg',lg]])for(const [key,value] of Object.entries(values)){
  const name='--'+prefix+'-'+key,valueCss=rgb(value);if(root.style.getPropertyValue(name)!==valueCss)root.style.setProperty(name,valueCss);
 }
 root.style.colorScheme=dark?'dark':'light';root.dataset.lgTheme=dark?'dark':'light';
 probe.remove();
}
function schedule() {
 if(pending)return;pending=true;
 requestAnimationFrame(()=>{pending=false;for(const [root,seen] of roots){
  if(root.isConnected){roots.set(root,true);paint(root);}else if(seen)roots.delete(root);
 }});
}
function start() {
 if(watching)return;watching=true;
 const options={attributes:true,attributeFilter:['class','style','data-theme','data-darkmode','data-mode']};
 const observer=new MutationObserver(schedule);observer.observe(document.documentElement,options);observer.observe(document.body,options);
 const headObserver=new MutationObserver(schedule);headObserver.observe(document.head,{childList:true,subtree:true,attributes:true,attributeFilter:['href','media','disabled']});
 document.addEventListener('load',e=>{if(e.target.tagName==='LINK'&&e.target.rel==='stylesheet')schedule();},true);
 const media=matchMedia('(prefers-color-scheme: dark)');if(media.addEventListener)media.addEventListener('change',schedule);
}
return baseclass.extend({attach(root){
 for(const [old,seen] of roots)if(seen&&!old.isConnected)roots.delete(old);
 roots.set(root,false);paint(root);start();schedule();
 return ()=>roots.delete(root);
}});
