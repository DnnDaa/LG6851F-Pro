'use strict';
'require baseclass';
function show(v){return v==null?'--':String(v);}
function state(c){return c.role==='pcc'?'主载波':c.state==null?'未返回':'状态 '+c.state;}
return baseclass.extend({
 summary:function(ca){
  if(!ca?.valid)return ca?.reason==='offline'?'无':'待更新';
  if(ca.dl_total_mhz!=null)return ca.dl_total_mhz+' MHz';
  return '频宽待确认';
 },
 description:function(ca){
  if(!ca?.valid)return ca?.reason==='offline'?'模组未联网':'正在读取模组主、副载波配置';
  var s='模组返回 '+ca.count+' 个载波 · 下行配置频宽';
  if(ca.dl_total_mhz!=null)s+=' '+ca.dl_total_mhz+' MHz';
  else s+='尚未全部确认（已确认 '+(ca.dl_known_mhz||0)+' MHz）';
  return s+' · '+Math.max(0,Math.round(ca.age||0))+' 秒前';
 },
 frequencies:function(ca){
  if(!ca?.valid)return ca?.reason==='offline'?'无':'待更新';
  return ca.carriers.map(c=>(c.rat==='nr'?'n':'B')+c.band+' '+c.arfcn+(c.role==='pcc'?' ★':'')).join(' + ');
 },
 rows:function(ca){
  if(!ca?.valid||!ca.carriers?.length)return [E('tr',{},E('td',{colspan:9},this.description(ca)))];
  var secondary=0;
  return ca.carriers.map(c=>E('tr',{},[
   c.role==='pcc'?'主载波':'副载波 '+(++secondary),
   (c.rat==='nr'?'NR · n':'LTE · B')+c.band,c.arfcn,c.pci,
   c.dl_bw_mhz==null?'未确认（编码 '+show(c.dl_bandwidth_code)+'）':c.dl_bw_mhz+' MHz',
   c.ul_configured===0?'未配置':c.ul_bw_mhz==null?'--':c.ul_bw_mhz+' MHz',
   show(c.dl_mcs),show(c.ul_mcs),state(c)
  ].map(v=>E('td',{},String(v)))));
 },
 table:function(){return E('div',{style:'overflow-x:auto'},E('table',{'class':'lg-cell-table',style:'min-width:780px'},[
  E('thead',{},E('tr',{},['载波','制式 / 频段','ARFCN','PCI','下行频宽','上行频宽','下行 MCS','上行 MCS','模组状态'].map(v=>E('th',{},v)))),E('tbody',{id:'lg-ca-rows'})]));}
});
