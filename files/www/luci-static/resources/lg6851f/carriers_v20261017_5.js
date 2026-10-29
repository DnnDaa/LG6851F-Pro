'use strict';
'require baseclass';
function show(v){return v==null?'--':String(v);}
return baseclass.extend({
 summary:function(ca){
  if(!ca?.valid)return ca?.reason==='offline'?'无':'待更新';
  if(ca.dl_total_mhz!=null)return ca.dl_total_mhz+' MHz';
  return '频宽待确认';
 },
 description:function(ca){
  if(ca?.reason==='offline')return '模组未联网';
  if(ca?.read?.state==='reading')return '读取中······';
  if(ca?.read?.state==='busy')return '接口忙，等待重试······';
  if(!ca?.valid)return ca?.read?.state==='error'?'读取失败，稍后重试':'读取中······';
  return ca.count+'CC'+(ca.dl_total_mhz!=null?' · '+ca.dl_total_mhz+' MHz':' · 频宽未确认');
 },
 frequencies:function(ca){
  if(!ca?.valid)return ca?.reason==='offline'?'无':'待更新';
  return ca.carriers.map(c=>(c.rat==='nr'?'n':'B')+c.band+' '+c.arfcn+(c.role==='pcc'?' ★':'')).join(' + ');
 },
 rows:function(ca){
  if(!ca?.valid||!ca.carriers?.length)return [];
  var number=0;
  return ca.carriers.map(c=>E('tr',{},[
   ++number,
   (c.rat==='nr'?'NR · n':'LTE · B')+c.band,c.arfcn,c.pci,
   c.dl_bw_mhz==null?'未确认（编码 '+show(c.dl_bandwidth_code)+'）':c.dl_bw_mhz+' MHz',
   c.ul_configured===0?'未配置':c.ul_bw_mhz==null?'--':c.ul_bw_mhz+' MHz',
   show(c.dl_mcs),show(c.ul_mcs)
  ].map(v=>E('td',{},String(v)))));
 },
 table:function(id){return E('div',{style:'overflow-x:auto'},E('table',{'class':'lg-cell-table',style:'min-width:780px'},[
  E('thead',{},E('tr',{},['载波','制式 / 频段','ARFCN','PCI','下行频宽','上行频宽','下行 MCS','上行 MCS'].map(v=>E('th',{},v)))),E('tbody',{id:id||'lg-ca-rows'})]));}
});
