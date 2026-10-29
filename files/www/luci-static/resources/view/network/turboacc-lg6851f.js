/* SPDX-License-Identifier: GPL-2.0-only
 * TurboACC LG6851F adaptation of chenmozhijin/turboacc controls.
 * Vendor HNAT is independent of TCP congestion control and qdisc selection.
 */
'use strict';
'require view';
'require rpc';
'require ui';
'require poll';
'require dom';

var getStatus = rpc.declare({object:'luci.turboacc',method:'status',expect:{'':{}}});
var applySettings = rpc.declare({object:'luci.turboacc',method:'apply',params:['sw','hw','cc','qdisc'],expect:{'':{}}});
var setNAT = rpc.declare({object:'luci.turboacc',method:'set_nat',params:['mode'],expect:{'':{}}});
var natNames = ['默认 NAT（原有行为）','全锥形 NAT（IPv4 UDP）','对称 NAT（随机端口映射）'];
function state(value) { return value ? '已启用' : '未启用'; }
function option(value, label, current) { return E('option', {value:value,selected:value===current?'':null}, label); }
return view.extend({
 handleSaveApply:null,handleSave:null,handleReset:null,
 load:function(){return getStatus();},
 render:function(data){
  var alive=true,busy=false,controls={},statusNode=E('div');
  function paint(s){
   var ppe=(s.ppe||[]).map(function(p){return 'PPE'+p.id+'：'+p.bound+' / '+p.total;}).join('；') || '暂未取得计数';
   var kinds={};(s.eth0_qdisc||'').split('\n').forEach(function(line){var m=/^qdisc (\S+)/.exec(line);if(m)kinds[m[1]]=(kinds[m[1]]||0)+1;});
   var queue=Object.keys(kinds).map(function(k){return k.toUpperCase()+' × '+kinds[k];}).join('；') || '暂未取得队列';
   var rows=[['原厂 HNAT 引擎',s.native_hnat_loaded?'已加载':'未加载'],['PPE 已绑定连接',ppe],
    ['WED / WARP 驱动',s.wed_loaded?'已加载':'未加载'],['CPU → Wi-Fi 加速',state(s.cpu_wifi)],['同 BSSID 转发加速',state(s.intrabss)],
    ['防火墙软件流量表',s.flowtable_present?'已建立':'未建立'],['流量表硬件卸载请求',state(s.flowtable_hardware_requested)],
    ['TCP 拥塞控制',s.cc||'未知'],['默认队列算法',s.qdisc||'未知'],['eth0 实际队列',queue],
    ['当前 IPv4 NAT 模式',s.nat_engine_ready?(natNames[s.nat_mode]||'未知'):'原有 NAT；兼容扩展尚未加载']];
   dom.content(statusNode,E('table',{'class':'table'},rows.map(function(r){return E('tr',{'class':'tr'},[E('td',{'class':'td'},r[0]),E('td',{'class':'td'},r[1])]);})));
  }
  var mode=data.configured_hw&&data.configured_sw?'hardware':data.configured_sw?'software':'off';
  controls.mode=E('select',{'class':'cbi-input-select'},[option('hardware','软件 + 硬件流量分载',mode),option('software','仅软件流量分载',mode),option('off','关闭防火墙流量分载',mode)]);
  controls.cc=E('select',{'class':'cbi-input-select'},(data.cc_available||[]).filter(function(x){return ['bbr','cubic','reno'].includes(x);}).map(function(x){return option(x,x.toUpperCase(),data.cc);}));
  controls.qdisc=E('select',{'class':'cbi-input-select'},['cake','fq','fq_codel'].map(function(x){return option(x,x.toUpperCase(),data.qdisc);}));
  controls.nat=E('select',{'class':'cbi-input-select',disabled:data.nat_engine_ready?null:''},natNames.map(function(x,i){return option(String(i),x,String(data.nat_engine_ready?data.nat_mode:(data.nat_saved||0)));}));
  var natButton=E('button',{'class':'cbi-button cbi-button-apply',disabled:data.nat_engine_ready?null:'','click':ui.createHandlerFn(this,function(){
   if(busy)return;busy=true;natButton.disabled=true;
   return setNAT(Number(controls.nat.value)).then(function(r){
    if(!alive)return;if(!r.success)throw new Error(r.error||'NAT 模式应用失败');paint(r.status);ui.addNotification(null,E('p',{},r.message),'info');
   }).catch(function(e){if(alive)ui.addNotification(null,E('p',{},e.message),'error');}).finally(function(){busy=false;if(alive)natButton.disabled=false;});
  })},'应用 NAT 模式');
  var button=E('button',{'class':'cbi-button cbi-button-apply','click':ui.createHandlerFn(this,function(){
   if(busy)return;busy=true;button.disabled=true;
   return applySettings(controls.mode.value!=='off',controls.mode.value==='hardware',controls.cc.value,controls.qdisc.value).then(function(r){
    if(!alive)return;if(!r.success)throw new Error(r.error||'应用失败');paint(r.status);ui.addNotification(null,E('p',{},r.message||'设置已应用'),'info');
   }).catch(function(e){if(alive)ui.addNotification(null,E('p',{},e.message),'error');}).finally(function(){busy=false;if(alive)button.disabled=false;});
  })},'保存并应用');
  function field(label,node,hint){return E('div',{'class':'cbi-value'},[E('label',{'class':'cbi-value-title'},label),E('div',{'class':'cbi-value-field'},[node,E('div',{'class':'cbi-value-description'},hint)])]);}
  var updater=function(){
   if(!alive||busy||document.hidden)return;busy=true;
   return getStatus().then(function(s){if(alive)paint(s);}).catch(function(){}).finally(function(){busy=false;});
  };
  var leave=function(){if(!alive)return;alive=false;poll.remove(updater);window.removeEventListener('pagehide',leave);};
  this._cleanup=leave;window.addEventListener('pagehide',leave);poll.add(updater,10);
  paint(data);
  return E('div',{},[E('h2',{},'TurboACC 网络加速'),
   E('p',{},'LG6851F / MT6990 适配版。运行状态读取实际驱动、流量表和内核参数，配置与系统原有设置共用。'),statusNode,
   E('p',{},'原厂 HNAT/PPE/WED 由当前 SDK 驱动管理。防火墙流量分载开关只控制 fw4 流量表，不会卸载原厂 HNAT；关闭 fw4 分载不代表原厂硬件引擎已经关闭。'),
   E('h3',{},'加速与 TCP 设置'),
   field('防火墙流量分载',controls.mode,'硬件模式同时启用软件分载。原厂 HNAT 与 fw4 的运行状态在上方分别显示。'),
   field('TCP 拥塞控制',controls.cc,'主要作用于路由器本机 TCP；与 HNAT 加速开关独立。'),
   field('默认队列算法',controls.qdisc,'沿用现有队列管理：有线/蜂窝使用所选算法；Wi-Fi 的 CAKE 选择仍使用经过验证的 FQ_CODEL。自定义 QoS 队列不被覆盖。'),
   E('p',{},'硬件加速可能绕过软件整形；安装插件本身不能保证下载占满链路时游戏延迟不变。'),
   E('div',{'class':'cbi-page-actions'},button),
   E('h3',{},'NAT 模式'),
   field('IPv4 出站 NAT',controls.nat,data.nat_engine_ready?'全锥形模式开放 IPv4 UDP 已建立的映射；TCP 和 IPv6 沿用原行为。对称模式对新 IPv4 连接分配独立随机端口。':'需要先安装兼容 NAT 模块并重启；当前模式保持原样。'),
   E('p',{},'只影响本机 NAT 和新建连接，不会清空现有连接。蜂窝运营商或上级路由器的 NAT 仍可能限制公网入站，选择全锥形不代表公网一定变为 NAT1。'),
   E('div',{'class':'cbi-page-actions'},natButton)]);
 },
 unload:function(){if(this._cleanup)this._cleanup();}
});
