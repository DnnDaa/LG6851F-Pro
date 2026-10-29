'use strict';
'require view';
'require lg6851f.dashboard_cache_v20261022_44 as displayStore';
'require lg6851f.cells_v20261021_27 as cells';
'require lg6851f.carriers_v20261020_24 as CA';
'require lg6851f.theme_v20261015_1 as theme';
'require rpc';
'require lg6851f.wireless_v20261023_53 as sharedWireless';
'require fiberhome.common_v20261016_3 as common';
// View-local DOM access works on the detached tree before its first paint.
// Never mutate the shared module used by other LuCI pages.
var paintRoot=null,displayCache=null,M=Object.create(common);
M.neighborRows=function(c){var rows=cells.current(c);return rows.length?rows.map(function(r){var html=common.neighborRows({neigh:[Object.assign({},r,{serving:true})]});return r.serving?html:html.replace(' ★','');}).join(''):common.neighborRows({neigh:[]});};
M.v=function(id){return paintRoot?paintRoot.querySelector('#fhd-'+id):null;};
M.html=function(id,markup){var e=M.v(id);if(!e||(displayCache&&!displayCache.allowed(id,markup)))return;if(e._fhdHtml!==markup){e.innerHTML=markup;e._fhdHtml=markup;}if(displayCache)displayCache.remember(id);};
M.set=function(id,text,color){var e=M.v(id);if(!e||(displayCache&&!displayCache.allowed(id,text)))return;var value=text==null||text===''?'--':String(text);
 if(e.textContent!==value){e.textContent=value;delete e._fhdHtml;}if(color!==undefined)e.style.color=color;if(displayCache)displayCache.remember(id);};
M.spark=function(el){common.spark.apply(common,arguments);if(el&&displayCache&&el.id)displayCache.remember(el.id.replace(/^fhd-/,''));};
var callBootstrap=rpc.declare({object:'fiberhomedash',method:'bootstrap',expect:{}});
var callSignalRefresh=rpc.declare({object:'fiberhomedash',method:'refresh_signal',expect:{}});

/* FiberHome status dashboard -- the LuCI landing page (menu.d hangs it at admin/home).
 *
 * Layout: the serving-cell card at the top is the only card; the rest are full-width sections (link & traffic /
 * neighbor cells / Wi-Fi, LAN, device, SIM / quick controls). Locks, SMS and the AT terminal are in the Cellular
 * submenu.
 *
 * One prepared backend frame carries CPU, traffic, cellular and shared
 * wireless data. Bootstrap paints it before mount, then one request every 2s
 * reads the same cached source. AT/association collection keeps its own TTLs.
 * A neighbor row's Lock goes through lock_set cell (SFUN restarts the radio stack, about half a minute without
 * service); the lock state comes from lock_get (read once when the page loads, again after a lock). */

var RATE_WIN = 40;

/* The engineering port gives only MCS/BLER; the modulation is derived here from the common 3GPP MCS table 1,
 * without an extra AT request for a display item. LTE uplink has other MCS boundaries than downlink/NR. */
function modulation(mcs, rat, uplink) {
	if (mcs == null || isNaN(Number(mcs))) return '--';
	mcs = Number(mcs);
	if (rat === 'lte' && uplink) {
		if (mcs >= 0 && mcs <= 10) return 'QPSK';
		if (mcs <= 20) return '16QAM';
		if (mcs <= 28) return '64QAM';
	} else {
		if (mcs >= 0 && mcs <= 9) return 'QPSK';
		if (mcs <= 16) return '16QAM';
		if (mcs <= 28) return '64QAM';
	}
	return '--';
}

function radioMetricRows(label, rat, cell) {
	var dlMcs = cell && cell.dl_mcs != null ? cell.dl_mcs : null;
	var ulMcs = cell && cell.ul_mcs != null ? cell.ul_mcs : null;
	var dlBler = cell && cell.dl_bler != null ? cell.dl_bler : null;
	var ulBler = cell && cell.ul_bler != null ? cell.ul_bler : null;
	var prefix = label ? label + ' ' : '';
	var pair = function(a, b, suffix) {
		return (a != null ? a + suffix : '--') + ' / ' + (b != null ? b + suffix : '--');
	};
	return '<div class="fhd-r"><span class="fhd-k">' + prefix + _('Modulation DL/UL') + '</span>' +
		'<span class="fhd-v" title="' + _('Estimated from current MCS') + '">' + modulation(dlMcs, rat, false) + ' / ' + modulation(ulMcs, rat, true) + '</span></div>' +
		'<div class="fhd-r"><span class="fhd-k">' + prefix + _('MCS DL/UL') + '</span><span class="fhd-v">' + pair(dlMcs, ulMcs, '') + '</span></div>' +
		/* The BLER row is only drawn when the backend really has BLER values:
		 * this module has no AT getter for it, so it would always read "--". */
		((dlBler != null || ulBler != null)
			? '<div class="fhd-r"><span class="fhd-k">' + prefix + _('BLER DL/UL') + '</span><span class="fhd-v">' + pair(dlBler, ulBler, '%') + '</span></div>'
			: '');
}

function paintPlugPower(element, plug) {
    if (!element) return;
    clearTimeout(element._powerExpiry);
    plug = plug || {};
    var watts = plug.watts, age = Number(plug.age), interval = Number(plug.interval) || 15;
    var ttl = Math.max(45, interval * 3);
    var valid = plug.valid === true && plug.status === 'ok' && typeof watts === 'number' &&
        Number.isFinite(watts) && watts >= 0 && watts <= 10000 && plug.age != null &&
        Number.isFinite(age) && age >= 0 && age <= ttl;
    if(!valid&&displayCache&&!displayCache.allowed('plug-power',null))return;
    if(displayCache)displayCache.stamps.power=Date.now()-Math.max(0,age||0)*1000;
    element.hidden = !valid;
    // Explicit display survives themes which override the global [hidden] rule.
    element.style.display = valid ? '' : 'none';
    element.textContent = valid ? '整机功率 ' + watts.toFixed(1) + ' W' : '';
    element.style.color = M.qCol('good');
    element.title = valid ? '智能插座交流输入功率，包含适配器损耗；' + Math.floor(age) + '秒前采样' : '';
    if(valid&&displayCache)displayCache.remember('plug-power');
    if (valid) element._powerExpiry = setTimeout(function() {
        if(displayCache){displayCache.pending['plug-power']=true;displayCache.paintNotes();}
        else {element.hidden = true; element.style.display = 'none'; element.textContent = '';}
    }, Math.max(1, (ttl - age) * 1000));
}

function wifiSummary(wf) {
    wf = wf || {};
    if(wf.available===false)return {ssid5:'暂不可用',ssid24:'暂不可用',chan5:'--',chan24:'--',txpower5:'--',txpower24:'--',wenc:'--',whid:'--',wcountry:'--'};
    var radios = Array.isArray(wf.radios) ? wf.radios : [];
    if (!radios.length && wf.band && wf.up) radios = [wf];
    var bands = { '5': [], '24': [] }, result = {};
    radios.forEach(function(r) {
        if (!r || r.enabled === false || r.disabled === true || r.disabled === 1 || r.disabled === '1' || r.up === false) return;
        var band = String(r.band || '').toLowerCase().replace(/\s/g, '');
        if (/^(5g|5ghz|5)$/.test(band)) bands['5'].push(r);
        else if (/^(2g|2.4g|2.4ghz|2.4)$/.test(band)) bands['24'].push(r);
    });
    function unique(items) { return items.filter(function(v, i) { return items.indexOf(v) === i; }); }
    ['5', '24'].forEach(function(key) {
        var list = bands[key];
        result['ssid' + key] = unique(list.map(function(r) { return r.ssid; }).filter(function(v) { return typeof v === 'string' && v.length; })).join(' · ') || '无';
        result['chan' + key] = unique(list.map(function(r) { return /^\d+$/.test(String(r.channel)) && +r.channel > 0 ? String(+r.channel) : ''; }).filter(Boolean)).join(' · ') || (list.length ? '--' : '无');
        result['txpower' + key] = unique(list.map(function(r) { return typeof r.txpower === 'number' && Number.isFinite(r.txpower) ? r.txpower + 'dBm' + (Number.isInteger(r.power_level) && r.power_level >= 0 && r.power_level <= 3 ? '+Power' + r.power_level : '') : '--'; })).join(' · ') || '无';
    });
    function common(property, format) {
        var all = bands['5'].concat(bands['24']);
        if (!all.length) return '无';
        var values = unique(all.map(function(r) { return format(r[property]); }));
        if (values.length === 1) return values[0];
        return ['5', '24'].filter(function(k) { return bands[k].length; }).map(function(k) {
            return (k === '5' ? '5G ' : '2.4G ') + unique(bands[k].map(function(r) { return format(r[property]); })).join(' · ');
        }).join(' / ');
    }
    result.wenc = common('enc', function(v) { return v ? String(v).toUpperCase() : '--'; });
    result.whid = common('hidden', function(v) { return v === true || v === 1 || v === '1' ? 'YES' : 'NO'; });
    result.wcountry = common('country', function(v) { return v || '--'; });
    return result;
}

return view.extend({
	load: function() {
        // Warm entry still paints the browser cache and refreshes after 1s.
        // Cold entry reads one shared device frame before rendering; never
        // display an empty shell while an existing device cache is available.
        if(displayStore.hasStoredDisplay())return Promise.resolve(null);
        return callBootstrap().then(function(frame){
            return frame&&frame.status&&frame.cpu&&frame.cell&&Number.isFinite(Number(frame.sample_mono))?frame:null;
        }).catch(function(){return null;});
    },

	render: function(initial) {
        if(initial?.cell&&!initial.cell.health?.signal_valid&&initial.prefill_cell?.health?.signal_valid)initial=Object.assign({},initial,{cell:initial.prefill_cell});
		M.injectCss();
		var root = document.createElement('div');
		this._root = root; paintRoot=root;
		this._bootEl = root;
		root.className = 'fhd fhd-dashboard fhd-booting';
		root.innerHTML = this.html();
        // The whole display is restored before its first paint.
        ['cpu','ram','disk'].forEach(function(id){var bar=root.querySelector('#fhd-'+id+'-bar');if(bar)bar.style.width='0%';});
		this.wire(root);
        if(displayCache)displayCache.destroy();
        displayCache=displayStore.create(root);displayCache.start();
        var hasDisplayedCache=Object.keys(displayCache.items).length>0;
        var firstFrameStored=hasDisplayedCache;
        function saveFirstFrame(){
            if(!firstFrameStored&&displayCache&&Object.keys(displayCache.items).length){displayCache.flush();firstFrameStored=true;}
        }
        if(hasDisplayedCache){root.classList.remove('fhd-booting');this._bootEl=null;}
        paintPlugPower(root.querySelector('#fhd-plug-power'),initial&&initial.plug_power);
		var self = this;
        var generation=self._generation=(self._generation||0)+1;
        var alive=function(){return !self._stopped && self._generation===generation && self._root===root;};
		self._stopped = false; self._lanes = {};
        self._wifiUnsubscribe=sharedWireless.subscribe(function(frame){if(alive())self.paintWifi(frame&&frame.wifi);});
        setTimeout(function(){if(alive())self.paintWifi(sharedWireless.cached()&&sharedWireless.cached().wifi);},0); self._atomicSystem = true; self._cpuPercent = null;
        if(initial && initial.cpu && initial.system && initial.boot_id && Number.isFinite(initial.cpu.percent)){
            self._systemFrame=initial.system;
            self._cpuPercent=initial.cpu.percent;self._bootId=initial.boot_id;
            self.lastCpu={total:initial.cpu.total,idle:initial.cpu.idle};
            self.paintSystem(root);
            setTimeout(function(){if(!self._stopped && document.documentElement.contains(root)){
                self.updateConnections(initial.connections);self.updatePPE(initial.ppe);
            }},0);
        }
        // Commit the cached network and cellular frames while root is detached.
        if(initial?.wireless)sharedWireless.accept(initial.wireless);
        if(initial && initial.status){
            self._trafficStarted=true;self.update(initial.status);self.updateTraffic(initial.status.info);
        }
        if(initial && initial.cell)self.updateRealtime(initial);
        self.paintWifi(sharedWireless.cached()&&sharedWireless.cached().wifi);
        if(initial?.status)saveFirstFrame();
		/* One shared frame replaces four overlapping polls. Bootstrap already
         * painted it: wait one ordinary interval before requesting again. */
        var state=self._lanes.snapshot={busy:false,timer:null,failures:0,lastAt:Date.now(),mountWait:0};
        function schedule(delay){if(alive())state.timer=setTimeout(run,delay);}
        function run(){
            if(!alive()||state.busy)return;
            if(!document.documentElement.contains(root)){if(++state.mountWait<=20)schedule(50);return;}
            if(document.hidden){schedule(5000);return;}
            state.busy=true;state.lastAt=Date.now();
            Promise.resolve().then(callBootstrap).then(function(reply){
                if(!alive())return;
                if(!reply||!reply.status||!reply.cpu||!reply.cell)throw new Error('Invalid dashboard frame');
                if(reply.wireless)sharedWireless.accept(reply.wireless);
                self._trafficStarted=true;self.update(reply.status);
                self.updateTraffic(reply.status.info);self.updateRealtime(reply);saveFirstFrame();state.failures=0;
            }).catch(function(){if(alive()){state.failures++;self.refreshFailed('snapshot');}}).finally(function(){
                state.busy=false;
                schedule(Math.min(15000,2000*Math.pow(2,Math.min(3,state.failures))));
            });
        }
        state.run=run;
        // Paint the restored view first, then request the shared frame once.
        // Keep the cached first paint stable for one second before requesting.
        // Preloaded cold frame already used the first request; keep one lane.
        schedule(initial?2000:1000);
		self._onVisibility = function() {
            if(document.hidden){if(alive()&&displayCache)displayCache.flush();return;}
            if(!alive()||state.busy)return;
            clearTimeout(state.timer);
            // One catch-up request only, with no extra traffic/AT warm-up lane.
            schedule(Math.max(0,2000-(Date.now()-state.lastAt)));
        };
		document.addEventListener('visibilitychange', self._onVisibility);
        self._onPageHide=function(){if(alive()&&displayCache)displayCache.flush();};
        window.addEventListener('pagehide',self._onPageHide);
		theme.attach(root);
		return root;
	},

	unload: function() {
		this._stopped = true;
        if(displayCache){displayCache.destroy();displayCache=null;}
        if(this._wifiUnsubscribe)this._wifiUnsubscribe();
        var power = this._root && this._root.querySelector('#fhd-plug-power');
        if (power) clearTimeout(power._powerExpiry);
		Object.keys(this._lanes || {}).forEach(function(key) { clearTimeout(this._lanes[key].timer); }, this);
		document.removeEventListener('visibilitychange', this._onVisibility);
        window.removeEventListener('pagehide',this._onPageHide);
	},

	html: function() {
		return `
<style>
.fhd.fhd-dashboard #fhd-rsrp,.fhd.fhd-dashboard #fhd-dl,.fhd.fhd-dashboard #fhd-ul,.fhd.fhd-dashboard .fhd-kpi b,.fhd.fhd-dashboard .fhd-rows .v{font-variant-numeric:tabular-nums}
.fhd.fhd-dashboard .fhd-cellmeta{min-height:20px}
.fhd.fhd-dashboard #fhd-rsrp{min-width:5.6ch;text-align:right}
.fhd.fhd-dashboard .fhd-kpi b{min-height:1.25em;display:inline-block}

.fhd.fhd-dashboard .fhd-hero-l{min-width:0}
.fhd.fhd-dashboard .fhd-cellmeta{display:flex;flex-wrap:nowrap;align-items:center;gap:12px;line-height:20px;max-width:100%;overflow-x:auto;scrollbar-width:thin}
.fhd.fhd-dashboard .fhd-cellmeta .fhd-cellline{flex:0 0 auto;white-space:nowrap;line-height:20px}
.fhd.fhd-dashboard .fhd-cellmeta .fhd-live-state{display:inline-flex;flex:0 0 auto;align-items:center;gap:4px;line-height:20px;margin:0;padding:0;white-space:nowrap}
.fhd.fhd-dashboard .fhd-cellmeta .fhd-live-state[hidden]{display:none!important}
.fhd.fhd-dashboard .fhd-cache-dots{display:inline-flex;align-self:center;align-items:center;gap:3px;height:1em}
.fhd.fhd-dashboard .fhd-cache-dots i{width:3px;height:3px;display:inline-block;border-radius:50%;background:currentColor;animation:fhd-cache-wave44 1.1s ease-in-out infinite}
.fhd.fhd-dashboard .fhd-cache-dots i:nth-child(2){animation-delay:.14s}
.fhd.fhd-dashboard .fhd-cache-dots i:nth-child(3){animation-delay:.28s}
@keyframes fhd-cache-wave44{0%,60%,100%{transform:translateY(0);opacity:.35}30%{transform:translateY(-3px);opacity:1}}
@media(prefers-reduced-motion:reduce){.fhd.fhd-dashboard .fhd-cache-dots i{animation:none}}
</style>
<div class="fhd-card fhd-hero">
  <div class="fhd-hero-l">
    <div style="font-size:.78rem;color:var(--fhd-muted,var(--fhd-muted,#777))">
      <span class="fhd-dot" id="fhd-dot"></span><b id="fhd-host" style="color:var(--fhd-text,#222)">--</b>
      <span id="fhd-uptime"></span></div>
    <div class="fhd-rat" id="fhd-rat">--<span class="fhd-bars" id="fhd-bars"><i style="height:25%"></i><i style="height:45%"></i><i style="height:65%"></i><i style="height:85%"></i><i style="height:100%"></i></span></div>
    <div class="fhd-op" id="fhd-op">--</div>
    <div class="fhd-cellmeta"><span class="fhd-cellline" id="fhd-cellline"></span><span class="fhd-live-state" id="fhd-live-state" role="status" aria-live="off" hidden><span id="fhd-live-text"></span><span class="fhd-cache-dots" aria-hidden="true"><i></i><i></i><i></i></span></span></div>
  </div>
  <div class="fhd-hero-r">
    <div class="fhd-rsrp" id="fhd-rsrp">--</div>
    <div class="fhd-chips" id="fhd-metric-chips"></div>
  </div>
</div>

<div class="fhd-card fhd-body">

<div class="fhd-sec">
  <h3>${_('Link & traffic')}</h3>
  <div class="fhd-charts">
    <div class="fhd-chart" style="color:var(--fhd-brand,var(--fhd-brand,#2f7bf6))">
      <div class="t"><b id="fhd-dl">--</b><span>${_('Download rate')}</span></div>
      <div class="c" id="fhd-spark-dl"></div>
    </div>
    <div class="fhd-chart" style="color:var(--fhd-positive,#2FBF71)">
      <div class="t"><b id="fhd-ul">--</b><span>${_('Upload rate')}</span></div>
      <div class="c" id="fhd-spark-ul"></div>
    </div>
  </div>
  <div class="fhd-kpis">
    <div class="fhd-kpi"><b id="fhd-rx">--</b><span>${_('Total received')}</span></div>
    <div class="fhd-kpi"><b id="fhd-tx">--</b><span>${_('Total sent')}</span></div>
  </div>
  <div class="fhd-cols">
    <div>
      <div class="fhd-rows">
        <div class="fhd-r"><span class="fhd-k">IPv4 / IPv6</span><span class="fhd-v" id="fhd-ip">--</span></div>
        <div class="fhd-r"><span class="fhd-k">APN</span><span class="fhd-v" id="fhd-apn">--</span></div>
        <div class="fhd-r"><span class="fhd-k">${_('Session duration')}</span><span class="fhd-v" id="fhd-sess">--</span></div>
        <div class="fhd-r"><span class="fhd-k">${_('Registration')}</span><span class="fhd-v" id="fhd-reg">--</span></div>
        <div class="fhd-r"><span class="fhd-k">DNS</span><span class="fhd-v" id="fhd-dns">--</span></div>
      </div>
    </div>
    <div>
      <div class="fhd-rows" id="fhd-lteanchor"></div>
      <div class="fhd-rows" id="fhd-radio-metrics">
        <div class="fhd-r"><span class="fhd-k">${_('Modulation DL/UL')}</span><span class="fhd-v">-- / --</span></div>
        <div class="fhd-r"><span class="fhd-k">${_('MCS DL/UL')}</span><span class="fhd-v">-- / --</span></div>
        <div class="fhd-r"><span class="fhd-k">${_('BLER DL/UL')}</span><span class="fhd-v">-- / --</span></div>
      </div>
      <div class="fhd-rows">
        <div class="fhd-r"><span class="fhd-k">频宽</span><span class="fhd-v" id="fhd-bw">--</span></div>
        <div class="fhd-r"><span class="fhd-k">${_('Carrier aggregation')}</span><span class="fhd-v" id="fhd-ca">--</span></div>
        <div class="fhd-r"><span class="fhd-k" id="fhd-qci-label">QCI</span><span class="fhd-v" id="fhd-qci">--</span></div>
        <div class="fhd-r"><span class="fhd-k">${_('AMBR DL/UL')}</span><span class="fhd-v" id="fhd-ambr">--</span></div>
      </div>
    </div>
  </div>
</div>


<div class="fhd-sec">
  <h3>${_('Wi-Fi · LAN · Device · SIM')}</h3>
  <div class="fhd-temp"><span id="fhd-temps" style="display:contents"></span><span id="fhd-plug-power" hidden style="white-space:nowrap;display:none" title="小米智能插座的交流输入功率，包含适配器损耗；每15秒本地读取一次"></span></div>
  <div class="fhd-kpis" style="margin-top:8px">
    <div class="fhd-kpi"><b id="fhd-cpu">--</b><span>CPU</span><div class="fhd-meter"><i id="fhd-cpu-bar" style="background:var(--fhd-brand,var(--fhd-brand,#3b82f6))"></i></div></div>
    <div class="fhd-kpi"><b id="fhd-ram">--</b><span>${_('Memory')} · <span class="fhd-sub" id="fhd-ram-sub">--</span></span><div class="fhd-meter"><i id="fhd-ram-bar" style="background:var(--fhd-info,#0ea5e9)"></i></div></div>
    <div class="fhd-kpi"><b id="fhd-disk">--</b><span>${_('Storage')}</span><div class="fhd-meter"><i id="fhd-disk-bar" style="background:var(--fhd-warning,#f59e0b)"></i></div></div>
    <div class="fhd-kpi" id="fhd-batt-kpi" style="display:none"><b id="fhd-batt">--</b><span id="fhd-batt-l">${_('Battery')}</span></div>
  </div>
  <div id="fhd-freqs" class="fhd-freqs"></div>
  <div id="fhd-ppe" class="fhd-ppe"></div>
  <div id="fhd-connections" class="fhd-ppe"></div>
  <div class="fhd-cols fhd-detail-cols">
    <div>
      <div class="fhd-rows">
        <div class="fhd-r"><span class="fhd-k">5G SSID</span><span class="fhd-v" id="fhd-ssid5">--</span></div>
        <div class="fhd-r"><span class="fhd-k">2.4G SSID</span><span class="fhd-v" id="fhd-ssid24">--</span></div>
        <div class="fhd-r"><span class="fhd-k">5G 信道</span><span class="fhd-v" id="fhd-chan5">--</span></div>
        <div class="fhd-r"><span class="fhd-k">2.4G 信道</span><span class="fhd-v" id="fhd-chan24">--</span></div>
        <div class="fhd-r"><span class="fhd-k">${_('Encryption')}</span><span class="fhd-v" id="fhd-wenc">--</span></div>
        <div class="fhd-r"><span class="fhd-k">${_('Hidden SSID')}</span><span class="fhd-v" id="fhd-whid">--</span></div>
        <div class="fhd-r"><span class="fhd-k">${_('Country')}</span><span class="fhd-v" id="fhd-wcountry">--</span></div>
        <div class="fhd-r"><span class="fhd-k">5G 发射功率</span><span class="fhd-v" id="fhd-txpower5" title="驱动报告的功率 + 当前配置的有效增益档位（关闭增益为Power0）；不是各MCS或仪器实测射频输出">--</span></div>
        <div class="fhd-r"><span class="fhd-k">2.4G 发射功率</span><span class="fhd-v" id="fhd-txpower24" title="驱动报告的功率 + 当前配置的有效增益档位（关闭增益为Power0）；不是各MCS或仪器实测射频输出">--</span></div>
        <div class="fhd-r"><span class="fhd-k">${_('LAN address')}</span><span class="fhd-v" id="fhd-lanip">--</span></div>
      </div>
      <section class="fhd-lease-box fhd-client-box"><div class="fhd-subheading"><b>${_('Wi-Fi clients')}</b><span id="fhd-wcl">--</span></div><div id="fhd-clist" class="fhd-device-list"></div></section>
    </div>
    <div>
      <div class="fhd-rows">
        <div class="fhd-r"><span class="fhd-k">${_('Device model')}</span><span class="fhd-v" id="fhd-model">--</span></div>
        <div class="fhd-r"><span class="fhd-k">${_('System')}</span><span class="fhd-v" id="fhd-fwos">--</span></div>
        <div class="fhd-r"><span class="fhd-k">${_('Modem')}</span><span class="fhd-v" id="fhd-modem">--</span></div>
        <div class="fhd-r"><span class="fhd-k">运营商</span><span class="fhd-v" id="fhd-carr">--</span></div>
        <div class="fhd-r"><span class="fhd-k">PLMN</span><span class="fhd-v" id="fhd-plmn">--</span></div>
        <div class="fhd-r"><span class="fhd-k fhd-ident-label">IMEI <button type="button" class="fhd-ident-toggle" id="fhd-reveal" aria-pressed="false">${_('Show SIM identifiers')}</button></span><span class="fhd-v" id="fhd-imei">--</span></div>
        <div class="fhd-r"><span class="fhd-k">IMSI</span><span class="fhd-v" id="fhd-imsi">--</span></div>
        <div class="fhd-r"><span class="fhd-k">ICCID</span><span class="fhd-v" id="fhd-iccid">--</span></div>
        <div class="fhd-r"><span class="fhd-k">${_('Module')}</span><span class="fhd-v" id="fhd-fwmodel">--</span></div>
        <div class="fhd-r"><span class="fhd-k">${_('Firmware')}</span><span class="fhd-v" id="fhd-fw">--</span></div>
      </div>
      <section class="fhd-lease-box"><div class="fhd-subheading"><b>DHCP 租约</b><span id="fhd-wleases">--</span></div><div id="fhd-leases" class="fhd-device-list"></div></section>
    </div>
  </div>
</div>


<!-- The FiberHome quick controls were dropped with the port: this panel is
     read-only and the A slot keeps its vendor pages for anything that writes. -->
<div class="fhd-sec">
  <h3>当前小区</h3>
  <div class="fhd-scroll">
  <table class="fhd-table"><thead><tr><th>${_('RAT/Band')}</th><th>PCI</th><th>${_('Frequency')}</th><th>RSRP</th><th>RSRQ</th><th>SINR</th></tr></thead>
  <tbody id="fhd-neigh"><tr><td colspan="6" style="color:var(--fhd-muted,var(--fhd-muted,#777))">--</td></tr></tbody></table>
  </div>
</div>

</div><!-- /fhd-body -->`;
	},

	wire: function(root) {
		var self = this;
		this.identShown = false;
		this.dlHist = []; this.ulHist = [];
		this.lastNet = null; this.lastCpu = null; this.lastFullTs = 0;
        this._netBoot=null;this._bootId=null;this._systemFrame=null;this._trafficWarming=false;
        this.lastCell=null;this.lastInfo=null;this.lastInfoTs=null;this._cellRowsKey=null;
        this._realtimeStarted=false;this._trafficStarted=false;this._realtimeMono=null;
        this._wifiFrame=null;this._paintedWifiKey=null;this._wifiKey=null;
		/* render() runs before the node is in the document, so look up relative to root (update, once mounted, looks in the whole document) */
		var q = function(id) { return root.querySelector('#fhd-' + id); };

		q('reveal').onclick = function() {
			self.identShown = !self.identShown;
			M.v('reveal').textContent = self.identShown ? _('Hide SIM identifiers') : _('Show SIM identifiers');
			M.v('reveal').setAttribute('aria-pressed',String(self.identShown));
			self.paintIdent(self.lastCell);
		};
	},

    paintWifi: function(wf) {
        if(!this._root||this._stopped)return;
        if(displayCache&&Object.keys(displayCache.items).some(function(id){return /^ssid|^clist$/.test(id);})&&
           (!wf||wf.available===false||(wf.boot_id&&this._bootId&&wf.boot_id!==this._bootId)||
            (wf.sample_id&&displayCache.scope?.wifiSample===wf.sample_id))){
            'ssid5 ssid24 chan5 chan24 txpower5 txpower24 wenc whid wcountry wcl clist'.split(' ').forEach(function(id){displayCache.pending[id]=true;});return;
        }
        if(displayCache){displayCache.stamps.wifi=Date.now();displayCache.scope=displayCache.scope||{};displayCache.scope.wifiSample=wf&&wf.sample_id;}
        if(this._wifiFrame?.boot_id&&wf?.boot_id&&this._wifiFrame.boot_id!==wf.boot_id)this._wifiFrame=null;
        if(wf&&wf.boot_id&&this._bootId&&wf.boot_id!==this._bootId)
            wf=Object.assign({},wf,{clients:[],clients_n:0,up:0,hostapd:0});
        if(wf&&Array.isArray(wf.clients)&&wf.clients_n!=null)this._wifiFrame=wf;
        wf=this._wifiFrame||wf||{};
        var key=JSON.stringify([wf.sample_id,wf.clients_n,wf.available]);
        if(key===this._paintedWifiKey)return;
        this._paintedWifiKey=key;

        var wireless = wifiSummary(wf);
        Object.keys(wireless).forEach(function(key) { M.set(key, wireless[key]); });
		M.set('wcl', wf.clients_n != null ? _('%s clients').format(wf.clients_n) : '正在更新');

        M.html('clist',(wf.clients||[]).map(function(cl){
            var level=cl.signal!=null?(cl.signal>=-55?'excellent':cl.signal>=-67?'good':cl.signal>=-80?'fair':'poor'):'unknown';
            var band=cl.band?M.esc(cl.band.replace(' GHz','G')):'';
            return '<div class="fhd-lease fhd-client"><div><b title="'+M.esc(cl.host||cl.ip||cl.mac||'未知设备')+'">'+M.esc(cl.host||cl.ip||cl.mac||'未知设备')+'</b><span>'+M.esc(cl.ip||'--')+'</span></div>'+
                '<div><small>'+M.esc(cl.mac||'--')+'</small><small class="fhd-client-signal" style="color:'+M.qCol(level)+'">'+(cl.signal!=null?M.esc(cl.signal)+' dBm':'--')+'</small></div>'+
                '<div class="fhd-client-rates"><small>↑ '+M.esc(cl.tx||'--')+' / ↓ '+M.esc(cl.rx||'--')+'</small><small>'+band+(cl.ifname?' · '+M.esc(cl.ifname):'')+'</small></div></div>';
        }).join('')||'<div class="fhd-note">'+(wf.clients_n==null?'正在读取无线客户端…':'当前没有已关联的无线客户端。')+'</div>');

    },

	paintIdent: function(cell) {
		var id = cell && cell.ident;
		var mask = function(s) {
			if (!s) return '--';
			return this.identShown ? s : s.substring(0, 4) + '****' + s.substring(s.length - 3);
		}.bind(this);
		M.set('imei', mask(id && id.imei));
		M.set('imsi', mask(id && id.imsi));
		M.set('iccid', mask(id && id.iccid));
		M.set('fwmodel', id ? (id.model || '--') : '--');
		M.set('fw', id ? (id.fw || '--') : '--');
	},

	repaintNeigh: function() {
		var el = M.v('neigh');
		if (el && this.lastCell) M.html('neigh',M.neighborRows(this.lastCell));
	},

	updateTraffic: function(i) {
		var net = (i.net && (i.net.mobile || i.net.sipa_eth0)) || null;
        if(displayCache){displayCache.stamps.traffic=Date.now()-Math.max(0,Number(i.age)||0)*1000;displayCache.clear.traffic=!!(net&&net.up===0);}
		var stamp = net && net.sample_ts != null ? net.sample_ts : i.ts;
		if (net && net.up && stamp != null) {
            var before=i.net_previous&&(i.net_previous.mobile||i.net_previous.sipa_eth0);
            if(i.boot_id&&this._netBoot&&i.boot_id!==this._netBoot)this.lastNet=null;
            if(i.boot_id)this._netBoot=i.boot_id;
            if(this.lastNet&&this.lastNet.iface===net.interface&&stamp<this.lastNet.ts)return;
            var previous = (this.lastNet&&stamp-this.lastNet.ts<=10?this.lastNet:null) || (before&&before.up&&before.interface===net.interface&&
                Number.isFinite(before.sample_ts)&&stamp>before.sample_ts&&stamp-before.sample_ts<=10
                ?{ts:before.sample_ts,rx:before.rx,tx:before.tx,iface:before.interface}:null);
			if (previous && previous.iface === net.interface && stamp > previous.ts) {
				var dt = stamp - previous.ts;
				var drx = net.rx - previous.rx, dtx = net.tx - previous.tx;
				if (dt <= 10 && drx >= 0 && dtx >= 0) {
					var dl = drx / dt, ul = dtx / dt;
					M.set('dl', M.fmtRate(dl)); M.set('ul', M.fmtRate(ul));
					this.dlHist.push(dl); this.ulHist.push(ul);
					if (this.dlHist.length > RATE_WIN) { this.dlHist.shift(); this.ulHist.shift(); }
					var peak = Math.max(1, Math.max.apply(null, this.dlHist.concat(this.ulHist)));
					M.spark(M.v('spark-dl'), this.dlHist, 0, peak, RATE_WIN);
					M.spark(M.v('spark-ul'), this.ulHist, 0, peak, RATE_WIN);
				} else { M.set('dl', '--'); M.set('ul', '--'); }
			} else if (!previous || previous.iface !== net.interface || stamp < previous.ts) {
				M.set('dl', '--'); M.set('ul', '--');
			}
			/* Equal timestamps are the same sample: retain the last measured rate. */
			if (!previous || stamp !== previous.ts || previous.iface !== net.interface)
				this.lastNet = { ts: stamp, rx: net.rx, tx: net.tx, iface: net.interface };
			M.set('rx', M.fmtBytes(net.rx)); M.set('tx', M.fmtBytes(net.tx));
		} else {
			this.lastNet = null;
			M.set('dl', '--'); M.set('ul', '--');
			M.set('rx', net ? M.fmtBytes(net.rx) : '--'); M.set('tx', net ? M.fmtBytes(net.tx) : '--');
		}

	},
	refreshFailed: function(lane) {
		if (this._stopped) return;
        if ((lane === 'status'||lane === 'snapshot')&&displayCache)Object.keys(displayCache.items).forEach(function(id){displayCache.pending[id]=true;});
        if (lane !== 'realtime'&&lane !== 'snapshot') return;
		var age = this._lastRealtimeAt ? Math.max(0, Math.round((Date.now() - this._lastRealtimeAt) / 1000)) : null;
		var note = M.v('live-state');
		if (note && !displayCache) { note.hidden = false; note.textContent = '刷新连接中断，正在重试' + (age != null ? ' · 上次成功 ' + age + ' 秒前' : ''); }
		if (M.v('cpu')) M.v('cpu').title = '采样连接暂不可用，显示上次有效值';
        if(displayCache){Object.keys(displayCache.items).forEach(function(id){displayCache.pending[id]=true;});displayCache.paintNotes();}
	},
	updateRealtime: function(reply) {
		if (this._stopped) return;
		if (!reply || !reply.cpu || !reply.cell || !Number.isFinite(Number(reply.sample_mono))) throw new Error('Invalid realtime reply');
		if (this._realtimeMono != null && reply.boot_id === this._bootId && Number(reply.sample_mono) < this._realtimeMono) return;
		this._realtimeMono = Number(reply.sample_mono); this._realtimeStarted = true;
		this._lastRealtimeAt = Date.now();
		if (reply.system) this._systemFrame = reply.system;
        if(displayCache)displayCache.stamps.system=Date.now();
		this.updateCPU(reply.cpu, reply.boot_id);
        this.paintSystem();
		var info = Object.assign({}, this.lastInfo || {}, {
			ts: reply.ts, host: reply.host, uptime: reply.uptime,
			modem: { alive: reply.cell.health && reply.cell.health.alive }
		});
		this.update({ info: info, cell: reply.cell, radio_only: true });
		this.updatePPE(reply.ppe);
        this.updateConnections(reply.connections);
        if(displayCache)displayCache.paintNotes();
	},
	updateCPU: function(cpu, bootId) {
		if (!cpu || cpu.total == null || cpu.idle == null || cpu.valid === 0) {
			if (M.v('cpu')) M.v('cpu').title = 'CPU 采样暂不可用，显示上次有效值';
			return;
		}
		var total = Number(cpu.total), idle = Number(cpu.idle);
		if (!Number.isFinite(total) || !Number.isFinite(idle) || total < 0 || idle < 0 || idle > total) return;
		var previous = this.lastCpu;
		if(bootId&&this._bootId&&bootId!==this._bootId){previous=null;this.lastNet=null;this._netBoot=bootId;}
		if (bootId) this._bootId = bootId;
		var delta = previous ? total - previous.total : 0;
		var idleDelta = previous ? idle - previous.idle : 0;
		if (previous && delta > 0 && idleDelta >= 0 && idleDelta <= delta) {
			var percent = Math.max(0, Math.min(100, Math.round((delta - idleDelta) * 100 / delta)));
			this._cpuPercent = percent;
            if (this._atomicSystem) this.paintSystem();
            else { M.set('cpu', percent + '%'); M.v('cpu-bar').style.width = percent + '%'; }
			M.v('cpu').title = '依据相邻两次实时 CPU 计数计算';
		 } else if (Number.isFinite(cpu.percent) && cpu.percent>=0 && cpu.percent<=100 && bootId) {
            this._cpuPercent=Math.round(cpu.percent);
            if(this._atomicSystem)this.paintSystem();
            M.v('cpu').title='依据同一次启动中的真实 CPU 计数计算；后续每 2 秒更新';
        } else if (!previous || delta < 0 || idleDelta < 0) {
			this._cpuPercent = null;
            M.set('cpu', '--'); if(!displayCache?.items['cpu-bar'])M.v('cpu-bar').style.width = '0%';
            if(this._atomicSystem)this.paintSystem();
			M.v('cpu').title = '正在建立 CPU 采样基线';
		}
		if (!previous || delta !== 0) this.lastCpu = { total: total, idle: idle };
		var freqs = Array.isArray(cpu.freqs) ? cpu.freqs : [];
		/* One cur/max frequency bar per cluster */
		M.html('freqs', freqs.map(function(f, n) {
			if (f.cur == null || f.max == null || !f.max) return '';
			var w = Math.max(2, Math.round(f.cur * 100 / f.max));
			return '<div class="fhd-freq"><span class="fhd-k">' + 'CPU频率' + (freqs.length > 1 ? ' ' + (f.cpus || n) : '') + '</span>' +
				'<div class="fhd-meter" style="flex:1;margin:4px 8px 0"><i style="width:' + w + '%;background:var(--fhd-brand,var(--fhd-brand,#3b82f6))"></i></div>' +
				'<span class="fhd-v" style="flex:0 0 auto">' + (f.cur / 1000).toFixed(0) + ' <span style="opacity:.55">/ ' + (f.max / 1000).toFixed(0) + ' MHz</span></span></div>';
		}).join(''));

	},
	paintSystem: function(root) {
        var frame=this._systemFrame||{},percent=this._cpuPercent;
        var v=root?function(id){return root.querySelector('#fhd-'+id);}:M.v;
        function paint(id,value){var e=v(id),bar=v(id+'-bar');if(e)M.set(id,value+'%');if(bar){bar.style.width=value+'%';if(displayCache)displayCache.remember(id+'-bar');}}
        if(Number.isFinite(percent)&&percent>=0&&percent<=100)paint('cpu',percent);
        var mem=frame.mem,disk=frame.storage;
        if(mem&&[mem.total_kb,mem.avail_kb].every(Number.isFinite)&&mem.total_kb>0&&mem.avail_kb>=0&&mem.avail_kb<=mem.total_kb){
            paint('ram',Math.round((mem.total_kb-mem.avail_kb)*100/mem.total_kb));
            var sub=v('ram-sub');if(sub)M.set('ram-sub',_('Total %s · free %s').format(M.fmtBytes(mem.total_kb*1024),M.fmtBytes(mem.avail_kb*1024)));
        }
        if(disk&&[disk.total_kb,disk.used_kb].every(Number.isFinite)&&disk.total_kb>0&&disk.used_kb>=0&&disk.used_kb<=disk.total_kb)paint('disk',Math.round(disk.used_kb*100/disk.total_kb));
    },
    updateConnections:function(connections){
        if(!connections || !Number.isInteger(connections.count) || connections.count<0)return;
        var count=connections.count,max=connections.max;
        var validMax=Number.isInteger(max)&&max>0;
        var percent=validMax?Math.round(count*10000/max)/100:0;
        var value=validMax?count+' / '+max+' ('+percent+'%)':count+' / 上限未取得';
        var tip='Linux conntrack 会话数，包含软件转发与硬件卸载会话；与 PPE 流表项数量含义不同。';
        M.html('connections','<div class="fhd-ppe-row"><span class="fhd-k">会话连接数</span>'+
            '<div class="fhd-ppe-meter" role="progressbar" aria-label="会话连接数" aria-valuemin="0" '+
            (validMax?'aria-valuemax="'+max+'" ':'')+'aria-valuenow="'+count+'" title="'+M.esc(tip)+'">'+
            '<i style="width:'+Math.min(100,percent)+'%"></i><span>'+value+'</span></div></div>');
    },
	updatePPE: function(ppe) {
		if (ppe == null) return;
		var entries = Array.isArray(ppe.entries) ? ppe.entries.slice().sort(function(a, b) { return a.id - b.id; }) : [];
		var rows = entries.filter(function(entry) {
			return Number.isInteger(entry.id) && entry.id >= 0 && Number.isInteger(entry.bound) && entry.bound >= 0 && Number.isInteger(entry.total) && entry.total > 0 && entry.bound <= entry.total;
		});
		M.html('ppe', rows.length ? rows.map(function(entry) {
			var percent = Math.round(entry.bound * 10000 / entry.total) / 100;
			var value = entry.bound + ' / ' + entry.total + ' (' + percent + '%)';
			var tip = '已绑定硬件流表项；双向连接可能占两个表项。未绑定 ' + entry.unbound + '，FIN ' + entry.fin;
			return '<div class="fhd-ppe-row"><span class="fhd-k">PPE' + entry.id + ' 已绑定连接数</span>' +
				'<div class="fhd-ppe-meter" role="progressbar" aria-label="PPE' + entry.id + ' 已绑定连接数" aria-valuemin="0" aria-valuemax="' + entry.total + '" aria-valuenow="' + entry.bound + '" title="' + M.esc(tip) + '">' +
				'<i style="width:' + percent + '%"></i><span>' + value + '</span></div></div>';
		}).join('') : '<div class="fhd-ppe-row"><span class="fhd-k">PPE 硬件加速</span><span class="fhd-note">不可读取</span></div>');
	},
	update: function(st) {
        if (Object.prototype.hasOwnProperty.call(st, 'plug_power')) {
            paintPlugPower(M.v('plug-power'),st.plug_power);
        }
		var i = st.info || {};
		if (!st.radio_only) this.lastInfo = i;
		if (i.ts && this._bootEl) {
			this._bootEl.classList.remove('fhd-booting');
			this._bootEl = null;
		}
		/* Fast tier on top: sig (serving cell/registration, every 2 s) overrides the same fields of the slow-tier cache c */
		var c = st.cell || this.lastCell || null;
        // Measured neighbors must never appear as a serving carrier or LTE anchor.
        if(c)c={...c,nr:c.nr?.serving===false?null:c.nr,lte:c.lte?.serving===false?null:c.lte};
		if (!st.radio_only && this._realtimeStarted && this.lastCell) c = this.lastCell;
		var s = st.sig || null;
		if (s && !s.error && (!c || !c.ts || (s.ts || 0) >= c.ts)) {
			if (c && s.partial) {
				/* The first core reply updates the registration, but a transient empty CESQ does not wipe the last engineering signal. */
				c = Object.assign({}, c, {
					ts: s.ts, cfun: s.cfun, reg: s.reg, reg5g: s.reg5g
				});
			} else c = c ? Object.assign({}, c, {
				ts: s.ts, cfun: s.cfun, reg: s.reg, reg5g: s.reg5g,
				sig_src: s.sig_src, sig: s.sig, lte: s.lte, nr: s.nr
			}) : s;
		}
		this.lastCell = c;
        // A status-only paint may still carry lastCell from a previous visit.
        // Only the actual cellular lane may release the restored signal.
        if(displayCache)displayCache.context(st.radio_only?c:null,i);
        // Cell availability also changes when fast signal validity changes.
        // Compare actual rendered inputs, not just the slow AT timestamp.
        var cellRowsKey = JSON.stringify([c && c.error, c && c.current_cells, c && c.neigh || []]);
        if (!displayCache?.holdSignal && cellRowsKey !== this._cellRowsKey) {
            this._cellRowsKey = cellRowsKey;
            this.repaintNeigh();
        }

		/* Slow-tier data (neighbors/carrier/identity) is redrawn only when the whole cache's timestamp changes */
		var fullTs = String((c && c.ts) || 0) + ':' + String(c && c.error || '');
		var slowChanged = fullTs !== this.lastFullTs;
		if(!displayCache?.holdSignal)this.lastFullTs = fullTs;
		/* The backend samples CPU/temperatures/memory/storage/Wi-Fi every few seconds
		 * (fiberhomedash.main.info_interval); the rates keep coming every poll. Redraw
		 * those panels only when the sampler really produced a new snapshot. */
		var infoTs = st.radio_only ? (this.lastInfoTs || 0) : (i.info_ts || i.ts || 0);
		var infoChanged = infoTs !== this.lastInfoTs;
		this.lastInfoTs = infoTs;

		M.set('host', i.host);
		M.set('uptime', i.uptime ? _('Uptime %s').format(M.fmtUptime(i.uptime)) : '');
		M.v('dot').className = 'fhd-dot' + ((c && c.health ? c.health.alive : i.modem && i.modem.alive) ? ' on' : '');
        if(displayCache)displayCache.remember('dot');

		if ((st.radio_only || !this._realtimeStarted) && !displayCache?.holdSignal) {
		var sig = c && !c.error ? (c.sig || {}) : {};
		var rsrp = sig.rsrp, rsrq = sig.rsrq, sinr = sig.sinr;
		/* On 4G sig is the LTE block (the backend's sig_src prefers NR > LTE > CESQ); c.lte.sinr is the same source
		 * at another timestamp -- only a fallback when sinr is missing, never shown next to it */
		if (sinr == null && c && c.lte && !c.nr && c.lte.sinr != null) sinr = c.lte.sinr;
		var label = M.qLabel(rsrp, rsrq, sinr), score = M.qScore({ rsrp: rsrp, rsrq: rsrq, sinr: sinr });
		var col = M.qCol(M.qLevel(rsrp, rsrq, sinr));

		var rat = '--';
		if (c && !c.error) {
			var nr = c.nr && c.nr.band && c.nr.serving !== false ? true : false;
			var lte = c.lte && c.lte.band && c.lte.serving !== false ? true : false;
			if (nr && lte) rat = '5G NSA';
			else if (nr) rat = '5G SA';
			else if (lte) {
				var act = (c.operator && c.operator.act) || (c.reg && c.reg.act);
				rat = (act == 13) ? '5G NSA' : (act == 11 || act == 18 || act == 19) ? '5G' : (act == 7 || act == 10) ? '4G' : (act >= 2 && act <= 6) ? '3G' : '4G';
			} else if (c.cfun === 0) rat = _('Radio off');
            else if([1,5,10].includes(c.reg?.stat)) {var act=c.operator?.act??c.reg?.act;rat=act===11?'5G SA':act===13?'5G NSA':act===7?'4G LTE':'--';}
		}
		if (c && c.error && (!displayCache?.items.rat || ['radio_off','not_registered'].includes(c.error))) { rat = ({ radio_off: _('Radio off'), not_registered: _('Not registered'), stale_signal: '信号待更新', signal_unavailable: '信号不可读取', registration_unavailable: '注册信息待更新', rpc_error: '管理连接中断' })[c.error] || _('No response'); col = M.qCol('poor'); }
		var ratEl = M.v('rat');
		if(!displayCache||displayCache.allowed('rat',rat)){ratEl.firstChild.nodeValue=rat;if(displayCache)displayCache.remember('rat');}
		ratEl.style.color = col;
		var bars = M.v('bars');
		if (bars&&(!displayCache?.items.rat||c?.health?.signal_valid||displayCache?.clear.signal)) {
			var n = score == null ? 0 : Math.max(1, Math.round(score / 2));
			Array.prototype.forEach.call(bars.children, function(b, idx) { b.className = idx < n ? 'on' : ''; });
		}

        if(displayCache&&rat!=='--'&&!c?.error)displayCache.remember('rat');
		var oper = M.carrierName(c && c.operator);
		/* With COPS empty (the transition after a restart), the PLMN comes from the first 5-6 digits of the IMSI */
		if (oper === '--' && c && c.ident && c.ident.imsi) {
			var imsi = c.ident.imsi;
			var plmn5 = imsi.substring(0, 5), plmn6 = imsi.substring(0, 6);
			oper = M.PLMN_CN[plmn5] || M.PLMN_CN[plmn6] || plmn5;
		}
		M.set('carr',oper);
		M.set('op', score != null ? _('%s · Signal %s %s/10').format(oper, label, score.toFixed(1)) : _('%s · Signal %s').format(oper, label));

		var cl = [];
		if (c && c.nr && c.nr.band) cl.push('n' + c.nr.band + (c.nr.bw_mhz ? ' · ' + c.nr.bw_mhz + ' MHz' : '') + ' · PCI ' + c.nr.pci + ' · ARFCN ' + c.nr.arfcn);
		if (c && c.lte && c.lte.band) cl.push(_('Anchor B%s').format(c.lte.band) + ' · PCI ' + c.lte.pci + ' · EARFCN ' + c.lte.earfcn +
			(c.lte.sinr != null ? ' · SINR ' + c.lte.sinr.toFixed(1) + ' dB' : ''));
		M.html('cellline', cl.map(M.esc).join(' · ') || '<span style="color:var(--fhd-muted,var(--fhd-muted,#777))">' + _('No serving cell') + '</span>');

		/* Render once: resetting text before a memoized HTML write loses unchanged readings. */
		M.html('rsrp', typeof rsrp === 'number' && Number.isFinite(rsrp)
			? rsrp.toFixed(1) + '<small> dBm</small>' : '--');
		M.v('rsrp').style.color = col;
		M.html('metric-chips', '<span class="fhd-q" style="background:color-mix(in oklab,' + col + ' 16%,transparent);color:' + col + '">' + label + '</span>' +
			(rsrq != null ? '<span class="fhd-tag">RSRQ ' + rsrq.toFixed(1) + '</span>' : '') +
			(sinr != null ? '<span class="fhd-tag">SINR ' + sinr.toFixed(1) + '</span>' : ''));

		/* -- link & traffic */
		var nrk = (c && c.nr) || null;
		var radioMetrics = '';
		if (c && c.nr && c.nr.band) radioMetrics += radioMetricRows('5G', 'nr', c.nr);
		if (c && c.lte && c.lte.band) radioMetrics += radioMetricRows('4G', 'lte', c.lte);
	M.html('radio-metrics', radioMetrics || radioMetricRows('', 'nr', null));
		M.set('bw', CA.summary(c && c.ca));
		M.v('bw').title = CA.description(c && c.ca) + '；主、副载波配置带宽之和，不代表瞬时调度资源';
		M.set('ca', CA.frequencies(c && c.ca));
		M.v('ca').title = CA.description(c && c.ca) + '；★ 为主载波，邻区不计入聚合';
		var qos = c && c.qos;
		M.set('qci-label', 'QCI');
		M.set('qci', qos && qos.qci != null ? qos.qci : '--');
		M.v('qci').title = qos && qos.cid != null ? 'CID ' + qos.cid + ' · ' + (qos.apn || '') : '当前数据承载没有返回可确认的 QoS 等级';
		M.set('ambr', qos && qos.dl != null ? qos.dl + ' / ' + qos.ul + ' Mbps' : '--');

		}
		if (!this._trafficStarted) this.updateTraffic(i);

		var w = i.wan || {};
		if (infoChanged) {
			M.html('ip', M.esc(w.ip4 || '--') + (w.ip6 ? '<br>' + M.esc(w.ip6) : ''));
			M.set('dns', w.dns || '--');
			M.set('apn', w.apn || '--');
			M.set('sess', w.uptime ? M.fmtUptime(w.uptime) : '--');
		}
		if ((st.radio_only || !this._realtimeStarted) && !displayCache?.holdSignal) {
		var regmap = { 0: _('Not registered'), 1: _('Registered'), 2: _('Searching'), 3: _('Registration denied'), 4: _('Unknown'), 5: _('Registered (roaming)'), 7: _('Emergency only'), 8: _('Emergency only'), 10: _('Registered') };
		var reg = '--';
		if (c && c.reg) {
			reg = regmap[c.reg.stat] || _('Status %s').format(c.reg.stat);
			if (c.reg.tac) reg += ' · TAC ' + c.reg.tac;
			if (c.reg5g && c.reg5g.stat == 1) reg += ' · 5G ' + regmap[c.reg5g.stat];
		} else if (c && c.error) reg = M.errText(c);
		M.set('reg', reg);

		var anchor = (c && c.lte && c.lte.band) ? c.lte : null;
		M.html('lteanchor', anchor ?
			'<div class="fhd-r"><span class="fhd-k">' + (c.nr && c.nr.band ? _('LTE anchor') : _('LTE link')) + '</span><span class="fhd-v">B' + M.esc(anchor.band) +
			' · RSRP ' + (anchor.rsrp != null ? anchor.rsrp.toFixed(1) : '--') +
			(anchor.sinr != null ? ' · SINR ' + anchor.sinr.toFixed(1) : '') +
			(anchor.ca ? ' · ' + M.esc(anchor.ca) : '') + '</span></div>' : '');

		/* -- slow-tier sections */
		if (slowChanged) {
			M.set('carr', oper);
			M.set('plmn', (c && c.operator && c.operator.plmn) ||
				(c && c.ident && c.ident.imsi ? c.ident.imsi.substring(0, 5) : '--'));
			this.paintIdent(c);
		}

		}
		/* -- Wi-Fi, LAN, device, SIM */
		i.wifi=this._wifiFrame||sharedWireless.cached()?.wifi||{};
		var wifiKey=JSON.stringify([i.wifi && i.wifi.sample_id,i.wifi && i.wifi.available]);
		var wifiChanged=!st.radio_only && wifiKey!==this._wifiKey;
		if (!st.radio_only) this._wifiKey=wifiKey;
		if (infoChanged || wifiChanged) {
        this.paintWifi(i.wifi);
		M.set('lanip', (i.lan && i.lan.ip) || '--');
		M.set('wleases', _('%s entries').format(i.lan ? i.lan.leases : '--'));

		/* Recent DHCP leases: shows who has been here even when nobody is connected */

  var leaseList=(i.lan && i.lan.list) || [];
  M.html('leases',leaseList.length ? leaseList.slice(0,8).map(function(l){
   return '<div class="fhd-lease"><div><b title="'+M.esc(l.host || l.ip || '未知设备')+'">'+M.esc(l.host || l.ip || '未知设备')+'</b><span>'+M.esc(l.ip || '')+'</span></div><div><small>'+M.esc(l.mac || '')+'</small><small>DHCP</small></div><div class="fhd-client-rates"><small>剩余租期</small><small>'+(l.left>=3600?Math.round(l.left/3600)+' 小时':Math.max(0,Math.round(l.left/60))+' 分钟')+'</small></div></div>';
  }).join('') : '<div class="fhd-note">暂无 DHCP 租约</div>');


		}
		if (infoChanged) {
		var t = i.temps || {};
        var source=t.sources || {};
        var tempSamples=displayCache?(displayCache.scope.temps=displayCache.scope.temps||{}):{},tempRetained=false;
        M.html('temps',[['CPU',t.cpu,source.cpu],['蜂窝模组',t.modem,source.modem],['WIFI温度',t.wifi,source.wifi]].map(function(x){
            var number=x[1]!=null && Number.isFinite(Number(x[1]))?Number(x[1]):null;
            if(number!=null)tempSamples[x[0]]={value:number,at:displayCache?.stamps.system||Date.now()};
            else if(tempSamples[x[0]]){number=tempSamples[x[0]].value;tempRetained=true;}
            var tip=x[2]&&x[2].length?'传感器：'+x[2].join('、')+'；取最高温':'未取得有效温度读数';
            if(x[0]==='WIFI温度')tip='MT6990 片内 connsys 连接子系统温度；并非外置 Wi-Fi 射频芯片温度。';
            var color=number==null?'var(--fhd-muted,#777)':M.qCol(number>=75?'poor':number>=60?'fair':'good');
            return '<span title="'+M.esc(tip)+'" style="color:'+color+';white-space:nowrap">'+x[0]+' '+(number==null?'--':number.toFixed(1)+'°C')+'</span>';
        }).join(''));
        if(displayCache&&displayCache.items.temps){var tempAges=Object.keys(tempSamples).map(function(k){return tempSamples[k].at;});if(tempAges.length)displayCache.items.temps.at=Math.min.apply(null,tempAges);displayCache.pending.temps=tempRetained;}


		if (!this._realtimeStarted && !this._atomicSystem) this.updateCPU(i.cpu);
		this.updatePPE(i.ppe);
        this.updateConnections(i.connections);
		if (!this._atomicSystem && i.mem && i.mem.total_kb) {
			var used = i.mem.total_kb - i.mem.avail_kb, pct = Math.round(used * 100 / i.mem.total_kb);
			M.set('ram', pct + '%'); M.v('ram-bar').style.width = pct + '%';
			M.set('ram-sub', _('Total %s · free %s').format(M.fmtBytes(i.mem.total_kb * 1024), M.fmtBytes(i.mem.avail_kb * 1024)));
		}
		if (!this._atomicSystem && i.storage && i.storage.total_kb) {
			var pct2 = Math.round(i.storage.used_kb * 100 / i.storage.total_kb);
			M.set('disk', pct2 + '%'); M.v('disk-bar').style.width = pct2 + '%';
		}
		/* Battery (U30 Air): capacity, then what it is doing and how many watts, the
		 * voltage, and the USB input where the charger reports it. No battery (F50):
		 * no tile. */
		var p = i.power || {}, bk = M.v('batt-kpi');
		if (bk) bk.style.display = p.present ? '' : 'none';
		if (p.present) {
			/* direction from status; for Unknown and the like from the sign of the current (positive into the
			 * battery), with the gauge's own 20 mA dead band. The 5.4 SQC charger says Unknown once full. */
			var pst = p.status, pcur = p.ua || 0, parts = [ _('Battery') ];
			var pdir = pst === 'Charging' ? 1 : pst === 'Discharging' ? -1 :
				(pst === 'Full' || pst === 'Not charging') ? 0 : (pcur >= 20000 ? 1 : pcur <= -20000 ? -1 : 0);
			var pw = p.w != null ? Number(p.w).toFixed(1) : null;
			if (pdir > 0) parts.push(pw != null ? _('charging %s W').format(pw) : _('charging'));
			else if (pdir < 0) parts.push(pw != null ? _('drawing %s W').format(pw) : p.usb ? _('not charging') : _('on battery'));
			else if (pst === 'Full' || (p.usb && p.capacity >= 100)) parts.push(_('full'));
			else if (pst === 'Not charging') parts.push(_('not charging'));
			if (p.volt != null && p.capacity != null) parts.push(p.volt + ' V');
			if (p.usb) parts.push(p.in_w != null ? _('USB in %s W').format(Number(p.in_w).toFixed(1)) +
				(p.in_volt != null ? ' (' + p.in_volt + ' V)' : '') : 'USB');
			M.set('batt', p.capacity != null ? p.capacity + '%' : (p.volt != null ? p.volt + ' V' : '--'));
			M.set('batt-l', parts.join(' · '));
		}
		M.set('model', i.model || '--');
		M.set('fwos', i.fw || '--');
		M.set('modem', (i.modem && i.modem.alive ? _('Online') : _('No response')) + (i.modem && i.modem.atd ? '' : ' · ' + _('AT adapter unavailable')));
		}
	}
});
