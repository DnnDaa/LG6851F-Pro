'use strict';
'require view';
'require uci';
'require fiberhome.common as M';

/* FiberHome status dashboard -- the LuCI landing page (menu.d hangs it at admin/home).
 *
 * Layout: the serving-cell card at the top is the only card; the rest are full-width sections (link & traffic /
 * neighbor cells / Wi-Fi, LAN, device, SIM / quick controls). Locks, SMS and the AT terminal are in the Cellular
 * submenu.
 *
 * One data source: ubus fiberhomedash status (the info snapshot + the fast-tier cellular cache sig + the slow-tier
 * cache cell). The page polls every 1.5 s by default: signal, rates and CPU refresh every round; neighbors, QoS
 * and identity are redrawn only when the slow tier's timestamp changes.
 * A neighbor row's Lock goes through lock_set cell (SFUN restarts the radio stack, about half a minute without
 * service); the lock state comes from lock_get (read once when the page loads, again after a lock). */

var DEFAULT_POLL_S = 1.5;
var RATE_WIN = 40;

function pollSeconds(value) {
	var seconds = Number(value);
	return Number.isFinite(seconds) && seconds >= 0.5 && seconds <= 60 ? seconds : DEFAULT_POLL_S;
}

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

return view.extend({
	load: function() { return Promise.resolve(); },

	render: function() {
		M.injectCss();
		var root = document.createElement('div');
		this._root = root;
		this._bootEl = root;
		root.className = 'fhd fhd-booting';
		root.innerHTML = this.html();
		this.wire(root);
		var self = this;
		var intervalMs = DEFAULT_POLL_S * 1000;
		var config = L.resolveDefault(uci.load('fiberhomedash')).then(function() {
			intervalMs = pollSeconds(uci.get('fiberhomedash', 'main', 'refresh_interval')) * 1000;
		});
		/* Read the full status once now; never run the sysinfo and status collections twice at the same time. */
		var first = L.resolveDefault(M.callStatus()).then(function(st) {
			self.update(st || {});
		});
		/* LuCI poll.add() truncates intervals to whole seconds. Use a one-shot
		 * timer so 1.5 s remains 1.5 s and slow requests never overlap. */
		function refresh() {
			if (!document.documentElement.contains(root)) return;
			if (document.hidden) { self._refreshTimer = setTimeout(refresh, intervalMs); return; }
			M.callStatus().then(function(st) { self.update(st || {}); }).catch(function() { self.update({ info: {}, cell: { error: 'rpc_error', sig: {}, neigh: [] } }); }).finally(function() {
				if (document.documentElement.contains(root))
					self._refreshTimer = setTimeout(refresh, intervalMs);
			});
		}
		Promise.all([first, config]).then(function() {
			self._refreshTimer = setTimeout(refresh, intervalMs);
		});
		return root;
	},

	unload: function() {
		clearTimeout(this._refreshTimer);
	},

	html: function() {
		return `
<div class="fhd-card fhd-hero">
  <div class="fhd-hero-l">
    <div style="font-size:.78rem;color:var(--text-muted,var(--text-light,#777))">
      <span class="fhd-dot" id="fhd-dot"></span><b id="fhd-host" style="color:var(--text,#222)">--</b>
      <span id="fhd-uptime"></span></div>
    <div class="fhd-rat" id="fhd-rat">--<span class="fhd-bars" id="fhd-bars"><i style="height:25%"></i><i style="height:45%"></i><i style="height:65%"></i><i style="height:85%"></i><i style="height:100%"></i></span></div>
    <div class="fhd-op" id="fhd-op">--</div>
    <div class="fhd-cellline" id="fhd-cellline"></div>
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
    <div class="fhd-chart" style="color:var(--brand,var(--primary,#2f7bf6))">
      <div class="t"><b id="fhd-dl">--</b><span>${_('Download rate')}</span></div>
      <div class="c" id="fhd-spark-dl"></div>
    </div>
    <div class="fhd-chart" style="color:var(--success,#2FBF71)">
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
        <div class="fhd-r"><span class="fhd-k">${_('Bandwidth')}</span><span class="fhd-v" id="fhd-bw">--</span></div>
        <div class="fhd-r"><span class="fhd-k">${_('Carrier aggregation')}</span><span class="fhd-v" id="fhd-ca">--</span></div>
        <div class="fhd-r"><span class="fhd-k" id="fhd-qci-label">QCI / 5QI</span><span class="fhd-v" id="fhd-qci">--</span></div>
        <div class="fhd-r"><span class="fhd-k">${_('AMBR DL/UL')}</span><span class="fhd-v" id="fhd-ambr">--</span></div>
      </div>
    </div>
  </div>
</div>


<div class="fhd-sec">
  <h3>${_('Wi-Fi · LAN · Device · SIM')}</h3>
  <div class="fhd-temp" id="fhd-temps"></div>
  <div class="fhd-kpis" style="margin-top:8px">
    <div class="fhd-kpi"><b id="fhd-cpu">--</b><span>CPU</span><div class="fhd-meter"><i id="fhd-cpu-bar" style="background:var(--brand,var(--primary,#3b82f6))"></i></div></div>
    <div class="fhd-kpi"><b id="fhd-ram">--</b><span>${_('Memory')} · <span class="fhd-sub" id="fhd-ram-sub">--</span></span><div class="fhd-meter"><i id="fhd-ram-bar" style="background:var(--info,#0ea5e9)"></i></div></div>
    <div class="fhd-kpi"><b id="fhd-disk">--</b><span>${_('Storage')}</span><div class="fhd-meter"><i id="fhd-disk-bar" style="background:var(--warning,#f59e0b)"></i></div></div>
    <div class="fhd-kpi" id="fhd-batt-kpi" style="display:none"><b id="fhd-batt">--</b><span id="fhd-batt-l">${_('Battery')}</span></div>
  </div>
  <div id="fhd-freqs" class="fhd-freqs"></div>
  <div class="fhd-cols">
    <div>
      <div class="fhd-rows">
        <div class="fhd-r"><span class="fhd-k">SSID</span><span class="fhd-v" id="fhd-ssid">--</span></div>
        <div class="fhd-r"><span class="fhd-k">${_('Channel')}</span><span class="fhd-v" id="fhd-chan">--</span></div>
        <div class="fhd-r"><span class="fhd-k">${_('Encryption')}</span><span class="fhd-v" id="fhd-wenc">--</span></div>
        <div class="fhd-r"><span class="fhd-k">${_('Hidden SSID')}</span><span class="fhd-v" id="fhd-whid">--</span></div>
        <div class="fhd-r"><span class="fhd-k">${_('Country')}</span><span class="fhd-v" id="fhd-wcountry">--</span></div>
        <div class="fhd-r"><span class="fhd-k">${_('AP status')}</span><span class="fhd-v" id="fhd-whostapd">--</span></div>
        <div class="fhd-r"><span class="fhd-k">${_('USB network')}</span><span class="fhd-v" id="fhd-wusb">--</span></div>
        <div class="fhd-r"><span class="fhd-k">${_('Connection tracking')}</span><span class="fhd-v" id="fhd-conntrack">--</span></div>
        <div class="fhd-r"><span class="fhd-k">${_('LAN address')}</span><span class="fhd-v" id="fhd-lanip">--</span></div>
        <div class="fhd-r"><span class="fhd-k">${_('Wi-Fi clients')}</span><span class="fhd-v" id="fhd-wcl">--</span></div>
        <div class="fhd-r"><span class="fhd-k">${_('DHCP leases')}</span><span class="fhd-v" id="fhd-wleases">--</span></div>
      </div>
      <div id="fhd-clist" style="margin-top:8px"></div>
    </div>
    <div>
      <div class="fhd-rows">
        <div class="fhd-r"><span class="fhd-k">${_('Device model')}</span><span class="fhd-v" id="fhd-model">--</span></div>
        <div class="fhd-r"><span class="fhd-k">${_('System')}</span><span class="fhd-v" id="fhd-fwos">--</span></div>
        <div class="fhd-r"><span class="fhd-k">${_('Modem')}</span><span class="fhd-v" id="fhd-modem">--</span></div>
        <div class="fhd-r"><span class="fhd-k">${_('Carrier')}</span><span class="fhd-v" id="fhd-carr">--</span></div>
        <div class="fhd-r"><span class="fhd-k">PLMN</span><span class="fhd-v" id="fhd-plmn">--</span></div>
        <div class="fhd-r"><span class="fhd-k">IMEI</span><span class="fhd-v" id="fhd-imei">--</span></div>
        <div class="fhd-r"><span class="fhd-k">IMSI</span><span class="fhd-v" id="fhd-imsi">--</span></div>
        <div class="fhd-r"><span class="fhd-k">ICCID</span><span class="fhd-v" id="fhd-iccid">--</span></div>
        <div class="fhd-r"><span class="fhd-k">${_('Module')}</span><span class="fhd-v" id="fhd-fwmodel">--</span></div>
        <div class="fhd-r"><span class="fhd-k">${_('Firmware')}</span><span class="fhd-v" id="fhd-fw">--</span></div>
      </div>
      <div class="fhd-chiprow"><span class="fhd-chip" id="fhd-reveal">${_('Show SIM identifiers')}</span></div>
    </div>
  </div>
  <div id="fhd-leases" style="margin-top:10px"></div>
</div>


<!-- The FiberHome quick controls were dropped with the port: this panel is
     read-only and the A slot keeps its vendor pages for anything that writes. -->
<div class="fhd-sec">
  <h3>${_('Neighbor cells')}</h3>
  <div class="fhd-scroll">
  <table class="fhd-table"><thead><tr><th>${_('RAT/Band')}</th><th>PCI</th><th>${_('Frequency')}</th><th>RSRP</th><th>RSRQ</th><th>SINR</th></tr></thead>
  <tbody id="fhd-neigh"><tr><td colspan="6" style="color:var(--text-muted,var(--text-light,#777))">--</td></tr></tbody></table>
  </div>
</div>

</div><!-- /fhd-body -->`;
	},

	wire: function(root) {
		var self = this;
		this.identShown = false;
		this.dlHist = []; this.ulHist = [];
		this.lastNet = null; this.lastCpu = null; this.lastFullTs = 0;
		/* render() runs before the node is in the document, so look up relative to root (update, once mounted, looks in the whole document) */
		var q = function(id) { return root.querySelector('#fhd-' + id); };

		q('reveal').onclick = function() {
			self.identShown = !self.identShown;
			M.v('reveal').textContent = self.identShown ? _('Hide SIM identifiers') : _('Show SIM identifiers');
			self.paintIdent(self.lastCell);
		};
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
		if (el && this.lastCell) el.innerHTML = M.neighborRows(this.lastCell);
	},

	update: function(st) {
		var i = st.info || {};
		this.lastInfo = i;
		if (i.ts && this._bootEl) {
			this._bootEl.classList.remove('fhd-booting');
			this._bootEl = null;
		}
		/* Fast tier on top: sig (serving cell/registration, every 1.5 s) overrides the same fields of the slow-tier cache c */
		var c = st.cell || null;
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
		/* Slow-tier data (neighbors/carrier/identity) is redrawn only when the whole cache's timestamp changes */
		var fullTs = String((st.cell && st.cell.ts) || 0) + ':' + String(c && c.error || '');
		var slowChanged = fullTs !== this.lastFullTs;
		this.lastFullTs = fullTs;
		/* The backend samples CPU/temperatures/memory/storage/Wi-Fi every few seconds
		 * (fiberhomedash.main.info_interval); the rates keep coming every poll. Redraw
		 * those panels only when the sampler really produced a new snapshot. */
		var infoTs = i.info_ts || i.ts || 0;
		var infoChanged = infoTs !== this.lastInfoTs;
		this.lastInfoTs = infoTs;

		M.set('host', i.host);
		M.set('uptime', i.uptime ? _('Uptime %s').format(M.fmtUptime(i.uptime)) : '');
		M.v('dot').className = 'fhd-dot' + (i.modem && i.modem.alive ? ' on' : '');

		var sig = c && !c.error ? (c.sig || {}) : {};
		var rsrp = sig.rsrp, rsrq = sig.rsrq, sinr = sig.sinr;
		/* On 4G sig is the LTE block (the backend's sig_src prefers NR > LTE > CESQ); c.lte.sinr is the same source
		 * at another timestamp -- only a fallback when sinr is missing, never shown next to it */
		if (sinr == null && c && c.lte && !c.nr && c.lte.sinr != null) sinr = c.lte.sinr;
		var label = M.qLabel(rsrp, rsrq, sinr), score = M.qScore({ rsrp: rsrp, rsrq: rsrq, sinr: sinr });
		var col = M.qCol(M.qLevel(rsrp, rsrq, sinr));

		var rat = '--';
		if (c && !c.error) {
			var nr = c.nr && c.nr.band ? true : false;
			var lte = c.lte && c.lte.band ? true : false;
			if (nr && lte) rat = '5G NSA';
			else if (nr) rat = '5G SA';
			else if (lte) {
				var act = (c.operator && c.operator.act) || (c.reg && c.reg.act);
				rat = (act == 13) ? '5G NSA' : (act == 11 || act == 18 || act == 19) ? '5G' : (act == 7 || act == 10) ? '4G' : (act >= 2 && act <= 6) ? '3G' : '4G';
			} else if (c.cfun === 0) rat = _('Radio off');
		}
		if (c && c.error) { rat = ({ radio_off: _('Radio off'), not_registered: _('Not registered'), stale_signal: '信号待更新', signal_unavailable: '信号不可读取', registration_unavailable: '注册信息待更新', rpc_error: '管理连接中断' })[c.error] || _('No response'); col = M.qCol('poor'); }
		var ratEl = M.v('rat');
		ratEl.firstChild.nodeValue = rat;
		ratEl.style.color = col;
		var bars = M.v('bars');
		if (bars) {
			var n = score == null ? 0 : Math.max(1, Math.round(score / 2));
			Array.prototype.forEach.call(bars.children, function(b, idx) { b.className = idx < n ? 'on' : ''; });
		}

		var oper = M.carrierName(c && c.operator);
		/* With COPS empty (the transition after a restart), the PLMN comes from the first 5-6 digits of the IMSI */
		if (oper === '--' && c && c.ident && c.ident.imsi) {
			var imsi = c.ident.imsi;
			var plmn5 = imsi.substring(0, 5), plmn6 = imsi.substring(0, 6);
			oper = M.PLMN_CN[plmn5] || M.PLMN_CN[plmn6] || plmn5;
		}
		M.set('op', score != null ? _('%s · Signal %s %s/10').format(oper, label, score.toFixed(1)) : _('%s · Signal %s').format(oper, label));

		var cl = [];
		if (c && c.nr && c.nr.band) cl.push('n' + c.nr.band + (c.nr.bw_mhz ? ' · ' + c.nr.bw_mhz + ' MHz' : '') + ' · PCI ' + c.nr.pci + ' · ARFCN ' + c.nr.arfcn);
		if (c && c.lte && c.lte.band) cl.push(_('Anchor B%s').format(c.lte.band) + ' · PCI ' + c.lte.pci + ' · EARFCN ' + c.lte.earfcn +
			(c.lte.sinr != null ? ' · SINR ' + c.lte.sinr.toFixed(1) + ' dB' : ''));
		M.v('cellline').innerHTML = cl.map(M.esc).join('<br>') || '<span style="color:var(--text-muted,var(--text-light,#777))">' + _('No serving cell') + '</span>';

		M.set('rsrp', '--');
		if (rsrp != null) M.v('rsrp').innerHTML = rsrp.toFixed(1) + '<small> dBm</small>';
		M.v('rsrp').style.color = col;
		M.v('metric-chips').innerHTML =
			'<span class="fhd-q" style="background:color-mix(in oklab,' + col + ' 16%,transparent);color:' + col + '">' + label + '</span>' +
			(rsrq != null ? '<span class="fhd-tag">RSRQ ' + rsrq.toFixed(1) + '</span>' : '') +
			(sinr != null ? '<span class="fhd-tag">SINR ' + sinr.toFixed(1) + '</span>' : '');

		/* -- link & traffic */
		var nrk = (c && c.nr) || null;
		var radioMetrics = '';
		if (c && c.nr && c.nr.band) radioMetrics += radioMetricRows('5G', 'nr', c.nr);
		if (c && c.lte && c.lte.band) radioMetrics += radioMetricRows('4G', 'lte', c.lte);
	M.v('radio-metrics').innerHTML = radioMetrics || radioMetricRows('', 'nr', null);
		M.set('bw', nrk && nrk.bw_mhz ? nrk.bw_mhz + ' MHz' : (c && c.lte && c.lte.bw) || '--');
		/* Carrier aggregation: every frequency layer the radio reports, serving
		 * carrier first.  The module has no AT command for the SCell state, so
		 * this is what AT+ECELL actually measured. */
		var carr = (c && c.carriers) || [];
		M.set('ca', carr.length
			? carr.map(function(x) {
				return (x.rat == 'nr' ? 'n' : 'B') + x.band + ' ' + x.arfcn +
					(x.serving ? ' ★' : '');
			}).join(' + ')
			: '--');
		var qos = c && c.qos;
		M.set('qci-label', qos && qos.kind ? qos.kind : 'QCI / 5QI');
		M.set('qci', qos && qos.qci != null ? qos.qci : '--');
		M.v('qci').title = qos && qos.cid != null ? 'CID ' + qos.cid + ' · ' + (qos.apn || '') : '当前数据承载没有返回可确认的 QoS 等级';
		M.set('ambr', qos && qos.dl != null ? qos.dl + ' / ' + qos.ul + ' Mbps' : '--');

		var net = (i.net && (i.net.mobile || i.net.sipa_eth0)) || null;
		var stamp = net && net.sample_ts != null ? net.sample_ts : i.ts;
		if (net && net.up && stamp != null) {
			var previous = this.lastNet;
			if (previous && previous.iface === net.interface && stamp > previous.ts) {
				var dt = stamp - previous.ts;
				var drx = net.rx - previous.rx, dtx = net.tx - previous.tx;
				if (drx >= 0 && dtx >= 0) {
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

		var w = i.wan || {};
		if (infoChanged) {
			M.v('ip').innerHTML = M.esc(w.ip4 || '--') + (w.ip6 ? '<br>' + M.esc(w.ip6) : '');
			M.set('dns', w.dns || '--');
			M.set('apn', w.apn || '--');
			M.set('sess', w.uptime ? M.fmtUptime(w.uptime) : '--');
		}
		var regmap = { 0: _('Not registered'), 1: _('Registered'), 2: _('Searching'), 3: _('Registration denied'), 4: _('Unknown'), 5: _('Registered (roaming)'), 7: _('Emergency only'), 8: _('Emergency only'), 10: _('Registered') };
		var reg = '--';
		if (c && c.reg) {
			reg = regmap[c.reg.stat] || _('Status %s').format(c.reg.stat);
			if (c.reg.tac) reg += ' · TAC ' + c.reg.tac;
			if (c.reg5g && c.reg5g.stat == 1) reg += ' · 5G ' + regmap[c.reg5g.stat];
		} else if (c && c.error) reg = M.errText(c);
		M.set('reg', reg);

		var anchor = (c && c.lte && c.lte.band) ? c.lte : null;
		M.v('lteanchor').innerHTML = anchor ?
			'<div class="fhd-r"><span class="fhd-k">' + (c.nr && c.nr.band ? _('LTE anchor') : _('LTE link')) + '</span><span class="fhd-v">B' + M.esc(anchor.band) +
			' · RSRP ' + (anchor.rsrp != null ? anchor.rsrp.toFixed(1) : '--') +
			(anchor.sinr != null ? ' · SINR ' + anchor.sinr.toFixed(1) : '') +
			(anchor.ca ? ' · ' + M.esc(anchor.ca) : '') + '</span></div>' : '';

		/* -- slow-tier sections */
		if (slowChanged) {
			this.repaintNeigh();
			M.set('carr', oper);
			M.set('plmn', (c && c.operator && c.operator.plmn) ||
				(c && c.ident && c.ident.imsi ? c.ident.imsi.substring(0, 5) : '--'));
			this.paintIdent(c);
		}

		/* -- Wi-Fi, LAN, device, SIM */
		if (infoChanged) {
		var wf = i.wifi || {};
		M.set('ssid', wf.ssid || '--');
		M.set('chan', (wf.channel || '--') + (wf.band ? ' (' + wf.band + (wf.width ? ' · ' + wf.width : '') + ')' : ''));
		M.set('wenc', wf.enc || '--');
		M.set('whid', wf.hidden == 1 ? _('Hidden') : _('No'));
		M.set('wcountry', wf.country || '--');
		M.set('whostapd', wf.hostapd ? _('Running') : _('Not running'));
		M.set('wusb', (i.net && i.net.usb0 && i.net.usb0.up) ? _('Connected') : _('Disconnected'));
		M.set('conntrack', i.conns != null ? _('%s entries').format(i.conns) : '--');
		M.set('lanip', (i.lan && i.lan.ip) || '--');
		M.set('wcl', _('%s clients').format(wf.clients_n != null ? wf.clients_n : '--'));
		M.set('wleases', _('%s entries').format(i.lan ? i.lan.leases : '--'));
		M.v('clist').innerHTML = (wf.clients || []).map(function(cl) {
			var l = cl.signal != null ? (cl.signal >= -55 ? 'excellent' : cl.signal >= -67 ? 'good' : cl.signal >= -80 ? 'fair' : 'poor') : 'unknown';
			return '<div class="fhd-cli"><div class="t"><b>' + M.esc(cl.host || cl.ip || cl.mac) + '</b>' +
				(cl.signal != null ? '<span style="color:' + M.qCol(l) + ';font-variant-numeric:tabular-nums">' + cl.signal + ' dBm</span>' : '') +
				'</div><div class="s">' + (cl.ip ? M.esc(cl.ip) + ' · ' : '') + M.esc(cl.mac) +
				((cl.tx || cl.rx) ? ' · ↑' + M.esc(cl.tx || '--') + ' ↓' + M.esc(cl.rx || '--') : '') +
				(cl.conn ? ' · ' + M.esc(cl.conn) : '') + '</div></div>';
		}).join('') || '';
		/* Recent DHCP leases: shows who has been here even when nobody is connected */
		M.v('leases').innerHTML = (i.lan && i.lan.list && i.lan.list.length)
			? '<div class="fhd-note" style="margin:0 0 4px">' + _('Recent DHCP leases') + '</div>' +
				'<table class="fhd-table"><tbody>' +
				i.lan.list.slice(0, 8).map(function(l) {
					return '<tr><td>' + M.esc(l.host || l.ip || '?') + '</td><td>' + M.esc(l.ip || '') + '</td>' +
						'<td style="color:var(--text-subtle,var(--text-light,#999))">' + M.esc(l.mac) + '</td>' +
						'<td>' + (l.left >= 3600 ? _('%d h').format(Math.round(l.left / 3600)) : _('%d min').format(Math.max(0, Math.round(l.left / 60)))) + '</td></tr>';
				}).join('') + '</tbody></table>'
			: '';

		}
		if (infoChanged) {
		var t = i.temps || {};
		M.v('temps').innerHTML = [ [ 'SoC', t.soc ], [ 'CPU', t.cpu ], [ _('Modem'), t.modem ], [ _('Board'), t.board ] ]
			.filter(function(x) { return x[1] != null; })
			.map(function(x) {
				var lab = x[1] >= 75 ? 'poor' : x[1] >= 60 ? 'fair' : 'good';
				return '<span style="color:' + M.qCol(lab) + '">' + x[0] + ' ' + x[1] + '°C</span>';
			}).join('') || '<span style="color:var(--text-muted,var(--text-light,#777))">' + _('No temperature readings') + '</span>';

		if (i.cpu && this.lastCpu && i.cpu.total != null && this.lastCpu.total != null) {
			var dt2 = i.cpu.total - this.lastCpu.total, di = i.cpu.idle - this.lastCpu.idle;
			var pct = dt2 > 0 ? Math.max(0, Math.min(100, Math.round((dt2 - di) * 100 / dt2))) : null;
			if (pct != null) {
				M.set('cpu', pct + '%');
				M.v('cpu-bar').style.width = pct + '%';
			}
		}
		this.lastCpu = i.cpu || null;
		/* One cur/max frequency bar per cluster */
		M.v('freqs').innerHTML = ((i.cpu && i.cpu.freqs) || []).map(function(f, n) {
			if (f.cur == null || f.max == null || !f.max) return '';
			var w = Math.max(2, Math.round(f.cur * 100 / f.max));
			return '<div class="fhd-freq"><span class="fhd-k">' + _('Cluster %d').format(n) + '</span>' +
				'<div class="fhd-meter" style="flex:1;margin:4px 8px 0"><i style="width:' + w + '%;background:var(--brand,var(--primary,#3b82f6))"></i></div>' +
				'<span class="fhd-v" style="flex:0 0 auto">' + (f.cur / 1000).toFixed(0) + ' <span style="opacity:.55">/ ' + (f.max / 1000).toFixed(0) + ' MHz</span></span></div>';
		}).join('');

		if (i.mem && i.mem.total_kb) {
			var used = i.mem.total_kb - i.mem.avail_kb, pct = Math.round(used * 100 / i.mem.total_kb);
			M.set('ram', pct + '%'); M.v('ram-bar').style.width = pct + '%';
			M.set('ram-sub', _('Total %s · free %s').format(M.fmtBytes(i.mem.total_kb * 1024), M.fmtBytes(i.mem.avail_kb * 1024)));
		}
		if (i.storage && i.storage.total_kb) {
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
