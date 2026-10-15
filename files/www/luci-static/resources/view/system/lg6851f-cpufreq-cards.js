'use strict';
'require view';
'require rpc';
'require poll';
'require uci';
'require ui';

var config = 'lg6851f-cpufreq';
var labels = { quiet: '省电', balanced: '均衡', performance: '性能' };
var getStatus = rpc.declare({ object: 'lg6851f.cpufreq', method: 'status', expect: {} });
function mhz(v) { return v > 0 ? String(Math.round(v / 1000)) : '—'; }
function words(v) { return String(v || '').trim().split(/\s+/).filter(Boolean); }
function select(values, current, disabled, change) {
	var input = E('select', { 'class': 'cpu-select', 'disabled': disabled || null, 'change': change }, values.map(function(v) {
		return E('option', { 'value': String(v.value), 'selected': String(v.value) === String(current) || null }, v.label);
	}));
	input.value = String(current);
	return input;
}
var css = '.cpu-page{color:#e5edf7;font-size:12px}.cpu-card{background:#151820;border:1px solid #303640;border-radius:16px;padding:16px;margin-bottom:14px}.cpu-title{font-weight:600;color:#9ba8bb;margin:0 0 8px}.cpu-title:before{content:"";display:inline-block;width:6px;height:6px;background:#0fb5b1;border-radius:50%;margin-right:7px;vertical-align:middle}.cpu-muted{color:#7c899d;line-height:1.8;margin:7px 0 0}.cpu-buttons{display:flex;gap:8px;margin:10px 0 8px}.cpu-preset{background:#151820;color:#edf5ff;border:1px solid #303640;border-radius:10px;padding:8px 13px;font-weight:600;cursor:pointer}.cpu-preset.active{border-color:#00a6a3;background:#123234;color:#fff}.cpu-preset:disabled{opacity:.5;cursor:default}.cpu-table-wrap{overflow:auto}.cpu-table{border-collapse:collapse;width:100%;font-size:13px}.cpu-table th{font-size:12px;color:#94a2b6;font-weight:400;border-bottom:1px solid #303640;padding:0 4px 4px;text-align:right;white-space:nowrap}.cpu-table td{padding:5px 4px;text-align:right;white-space:nowrap}.cpu-table th:first-child,.cpu-table td:first-child{text-align:left}.cpu-select{background:#151820!important;color:#e5edf7!important;border:1px solid #303640!important;border-radius:6px;min-width:86px;width:auto;max-width:145px;height:29px;padding:2px 20px 2px 6px;font-size:12px}.cpu-warning{color:#f1ba74;margin:8px 0}.cpu-current{font-variant-numeric:tabular-nums}@media(max-width:600px){.cpu-card{padding:12px}.cpu-table{min-width:620px}}';

return view.extend({
	load: function() { return Promise.all([ getStatus(), uci.load(config) ]); },
	render: function(result) {
		var self = this, data = result[0];
		this.data = data;
		this.mode = uci.get(config, 'main', 'mode') || 'balanced';
		this.savedMode = this.mode;
		this.profiles = {};
		Object.keys(labels).forEach(function(mode) {
			self.profiles[mode] = {
				governor: uci.get(config, mode, 'governor') || 'schedutil',
				min_freq: uci.get(config, mode, 'min_freq') || '',
				max_freq: uci.get(config, mode, 'max_freq') || ''
			};
		});
		var enabled = !!data.supported && !!data.hardware_cooling_ready && L.hasViewPermission();
		var buttons = {};
		var saved = E('p', { 'class': 'cpu-muted' });
		var help = E('p', { 'class': 'cpu-muted' });
		var controls = E('tbody');
		this.runtime = E('p', { 'class': 'cpu-muted' });
		this.currentCell = E('td', { 'class': 'cpu-current', 'title': '从原厂 PLL 与分频寄存器解码的实际时钟' }, mhz(data.cpu_pll_cur_freq));
		var freqs = words(data.scaling_available_frequencies).map(Number).sort(function(a,b) { return a-b; });
		function update() {
			var p = self.profiles[self.mode];
			Object.keys(buttons).forEach(function(mode) { buttons[mode].classList.toggle('active', mode === self.mode); });
			saved.textContent = '已保存的配置：' + (labels[self.savedMode] || self.savedMode) + (self.mode !== self.savedMode ? '　｜　待保存：' + labels[self.mode] : '');
			function updateHelp() {
				var value = data.cpuinfo_max_freq ? Math.round(Number(p.max_freq) * 100 / data.cpuinfo_max_freq) : 0;
				help.textContent = '所选范围：' + mhz(p.min_freq) + '～' + mhz(p.max_freq) + ' MHz，最高约为硬件上限的 ' + value + '%。' + (p.governor === 'performance' ? '保持所选最高频率，温控仍可降频。' : '按负载动态调频。');
			}
			updateHelp();
			var gov = select(words(data.scaling_available_governors).map(function(g) { return { value: g, label: g }; }), p.governor, !enabled, function(ev) { p.governor = ev.target.value; updateHelp(); });
			function freqInput(key) {
				return select(freqs.map(function(f) { return { value: f, label: mhz(f) }; }), p[key], !enabled, function(ev) { p[key] = ev.target.value; updateHelp(); });
			}
			controls.replaceChildren(E('tr', {}, [
				E('td', {}, data.related_cpus === '0 1 2 3' ? 'cpu0–3' : 'cpu' + words(data.related_cpus).join(',')),
				E('td', {}, gov), self.currentCell,
				E('td', {}, freqInput('min_freq')), E('td', {}, freqInput('max_freq')),
				E('td', {}, mhz(data.cpuinfo_max_freq))
			]));
		}
		var presetButtons = Object.keys(labels).map(function(mode) {
			buttons[mode] = E('button', { 'type': 'button', 'class': 'cpu-preset', 'disabled': !enabled || null,
				'click': function() { self.mode = mode; update(); } }, labels[mode]);
			return buttons[mode];
		});
		this.updateEditor = update;
		update();
		this.updateStatus(data);
		if (this.pollFn) poll.remove(this.pollFn);
		this.pollFn = function() { return getStatus().then(function(s) { self.data = s; self.updateStatus(s); }); };
		poll.add(this.pollFn, 5);
		return E('div', { 'class': 'cpu-page' }, [ E('style', {}, css),
			E('div', { 'class': 'cpu-card' }, [ E('p', { 'class': 'cpu-title' }, 'CPU 配置'),
				E('p', { 'class': 'cpu-muted' }, '配置会设置 CPU 调速器和每个集群的频率上下限，仅使用硬件频率表中的档位。电压由原厂硬件调频固件管理；温控可随时限制最高频率。'),
				E('div', { 'class': 'cpu-buttons' }, presetButtons), saved, help ]),
			E('div', { 'class': 'cpu-card' }, [ E('p', { 'class': 'cpu-title' }, '集群'),
				data.supported ? E('div', { 'class': 'cpu-table-wrap' }, E('table', { 'class': 'cpu-table' }, [
					E('thead', {}, E('tr', {}, ['核心', '调速器', '当前', '最低', '最高', '硬件最高'].map(function(x) { return E('th', {}, x); }))), controls ])) : E('p', { 'class': 'cpu-warning' }, data.error || 'CPUFreq policy0 不可用'),
				E('p', { 'class': 'cpu-muted' }, '频率单位为 MHz。四核共用 policy0；最低、最高和调速器编辑的是当前选中档位，保存并应用后生效。'), this.runtime ])
		]);
	},
	updateStatus: function(s) {
		this.currentCell.textContent = mhz(s.cpu_pll_cur_freq);
		this.runtime.textContent = s.supported ? '实际调速器：' + s.scaling_governor + '；请求频率：' + mhz(s.scaling_cur_freq) + ' MHz；有效范围：' + mhz(s.scaling_min_freq) + '～' + mhz(s.scaling_max_freq) + ' MHz。' + (s.hardware_cooling_ready ? ' CPU 温控已注册。' : ' CPU 温控未就绪，不能应用档位。') : '';
	},
	handleSave: function() {
		var self = this, data = this.data;
		if (!data.supported || !data.hardware_cooling_ready) return Promise.reject(new Error('硬件调频或 CPU 温控未就绪'));
		var freqs = words(data.scaling_available_frequencies), govs = words(data.scaling_available_governors);
		for (var mode of Object.keys(labels)) {
			var p = this.profiles[mode];
			if (freqs.indexOf(String(p.min_freq)) < 0 || freqs.indexOf(String(p.max_freq)) < 0 || Number(p.min_freq) > Number(p.max_freq) || govs.indexOf(p.governor) < 0)
				return Promise.reject(new Error(labels[mode] + '档位设置无效；最低频率不能高于最高频率，且必须使用硬件表中的档位'));
		}
		uci.set(config, 'main', 'mode', this.mode);
		Object.keys(labels).forEach(function(mode) { Object.keys(self.profiles[mode]).forEach(function(key) { uci.set(config, mode, key, self.profiles[mode][key]); }); });
		return uci.save().then(function() { self.savedMode = self.mode; self.updateEditor(); });
	},
	handleSaveApply: function(ev, mode) {
		return this.handleSave(ev).then(function() { return ui.changes.apply(mode === '0'); });
	},
	handleReset: function() {
		var self = this;
		return this.load().then(function(result) { return self.render(result); }).then(function(node) {
			var old = document.querySelector('.cpu-page'); if (old) old.replaceWith(node);
		});
	}
});
