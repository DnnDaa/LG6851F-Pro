'use strict';
'require view';
'require dom';
'require poll';
'require fs';
'require network';
'require ui';

function renderInclude(view, include, container, data) {
	if (view._statusStopped || include.hide || include.failed ||
	    (typeof include.load === 'function' && data == null))
		return false;

	var content = null;

	if (typeof include.render === 'function')
		content = include.render(data);
	else if (include.content != null)
		content = include.content;

	if (typeof include.oneshot === 'function') {
		include.oneshot(data);
		include.oneshot = null;
	}

	if (content == null)
		return false;

	container.parentNode.style.display = '';
	container.parentNode.classList.add('fade-in');
	dom.content(container, content);
	return true;
}

return view.extend({
	handleToggleSection: function(include, container, ev) {
		var btn = ev.currentTarget;

		include.hide = !include.hide;
		btn.setAttribute('data-style', include.hide ? 'active' : 'inactive');
		btn.setAttribute('class', include.hide ? 'label notice' : 'label');
		btn.firstChild.data = include.hide ? _('Show') : _('Hide');
		btn.blur();

		if (include.hide) {
			container.parentNode.style.display = 'none';
			localStorage.setItem(include.id, 'hide');
			return;
		}

		localStorage.removeItem(include.id);
		container.parentNode.style.display = '';
		dom.content(container, E('p', {}, E('em', { 'class': 'spinning' }, [ _('Collecting data...') ])));

		/* Hidden includes no longer consume RPC work during the first paint.
		 * Load one immediately when it becomes visible instead of waiting for
		 * the next poll. */
		return this.loadInclude(include, container);
	},

	loadIncludeData: function(include) {
		if (include._loadPromise)
			return include._loadPromise;
		if (this._statusStopped || include.hide || typeof include.load !== 'function')
			return Promise.resolve(null);
		var self = this;
		include._loadPromise = Promise.resolve().then(function() {
			if (self._statusStopped || include.hide) return null;
			return include.load();
		}).then(function(data) {
			include.failed = false;
			include._deferLoaded = true;
			return data;
		}, function() {
			include.failed = true;
			return null;
		}).finally(function() {
			include._loadPromise = null;
			include._deferPending = false;
		});
		return include._loadPromise;
	},

	loadInclude: function(include, container) {
		var self = this;
		return this.loadIncludeData(include).then(function(data) {
			if (self._statusStopped || include.hide)
				return data;
			try { renderInclude(self, include, container, data); }
			catch (error) { include.failed = true; }
			return data;
		});
	},
	invokeIncludesLoad: function(includes) {
		return Promise.all(includes.map(function(include) {
			if (include.hide || include._deferPending)
				return null;
			return this.loadIncludeData(include);
		}, this));
	},
	loadDeferredIncludes: function(includes, containers) {
		var self = this;
		var queue = [];

		for (var i = 0; i < includes.length; i++) {
			if (!includes[i].defer || includes[i].hide || includes[i]._deferLoaded)
				continue;

			includes[i]._deferPending = true;
			queue.push({ include: includes[i], container: containers[i] });
		}

		function schedule() {
			if (self._statusStopped || !queue.length)
				return;

			if (typeof window.requestIdleCallback === 'function')
				self._deferIdle = window.requestIdleCallback(run, { timeout: 800 });
			else
				self._deferTimer = setTimeout(run, 60);
		}

		function run() {
			self._deferIdle = null;
			self._deferTimer = null;

			if (self._statusStopped)
				return;

			var item = queue.shift();

			self.loadInclude(item.include, item.container).finally(function() {
				if (queue.length)
					schedule();
			});
		}

		schedule();
	},

	poll_status: function(includes, containers) {
		if (this._statusStopped || document.hidden || !includes.some(function(i) { return !i.hide; }))
			return Promise.resolve();
		if (this._statusPollPromise)
			return this._statusPollPromise;
		var self = this;

		this._statusPollPromise = Promise.resolve().then(function() { return network.flushCache(); }).then(function() {
			return self._statusStopped ? [] : self.invokeIncludesLoad(includes);
		}).then(function(results) {
			if (self._statusStopped || document.hidden)
				return;
			for (var i = 0; i < includes.length; i++) {
				var include = includes[i];
				if (include.hide || include._deferPending) continue;
				try { renderInclude(self, include, containers[i], results ? results[i] : null); }
				catch (renderError) { include.failed = true; }
			}

			var ssi = document.querySelector('div.includes');

			if (ssi) {
				ssi.style.display = '';
				ssi.classList.add('fade-in');
			}
		}).catch(function() {
			// Preserve the existing DOM if a shared network refresh fails.
		}).finally(function() {
			self._statusPollPromise = null;
		});
		return this._statusPollPromise;
	},

	load: function() {
		return L.resolveDefault(fs.list('/www' + L.resource('view/status/include')), []).then(function(entries) {
			var files = entries.filter(function(entry) {
				return entry.type === 'file' && entry.name.match(/\.js$/);
			}).sort(function(a, b) {
				return a.name.localeCompare(b.name);
			});

			return Promise.all(files.map(function(entry) {
				var name = 'view.status.include.' + entry.name.replace(/\.js$/, '');

				return Promise.resolve(L.require(name)).then(function(include) {
					/* Port and Wi-Fi status fan out to several services. Keep
					 * them out of the first concurrent burst and load each one
					 * during browser idle time. */
					if (/^(29_ports|60_wifi)\.js$/.test(entry.name) || entry.name === '60_wifi.js')
						include.defer = true;

					return include;
				});
			}));
		});
	},

	render: function(includes) {
		var rv = E([]);
		var containers = [];

		this._statusStopped = false;

		for (var i = 0; i < includes.length; i++) {
			var title = null;

			if (includes[i].title != null)
				title = includes[i].title;
			else
				title = String(includes[i]).replace(/^\[ViewStatusInclude\d+_(.+)Class\]$/, function(m, n) {
					return n.replace(/(^|_)(.)/g, function(m, s, c) {
						return (s ? ' ' : '') + c.toUpperCase();
					});
				});

			includes[i].id = title;
			includes[i].hide = localStorage.getItem(includes[i].id) === 'hide';

			var container = E('div');

			rv.appendChild(E('div', { 'class': 'cbi-section', 'style': 'display: none' }, [
				E('div', { 'class': 'cbi-title' }, [
					E('h3', { 'style': 'display: flex; justify-content: space-between' }, [
						title || '-',
						E('span', {
							'class': includes[i].hide ? 'label notice' : 'label',
							'role': 'button',
							'style': 'display: flex; align-items: center; justify-content: center; min-width: 4em',
							'data-style': includes[i].hide ? 'active' : 'inactive',
							'data-indicator': 'poll-status',
							'data-clickable': 'true',
							'click': ui.createHandlerFn(this, 'handleToggleSection', includes[i], container)
						}, [ _(includes[i].hide ? 'Show' : 'Hide') ])
					])
				]),
				container
			]));
			containers.push(container);
		}

		return this.poll_status(includes, containers).then(L.bind(function() {
			this.loadDeferredIncludes(includes, containers);
			this._statusPollFn = L.bind(this.poll_status, this, includes, containers);
			/* The dedicated dashboard owns high-frequency telemetry. Keep the
			 * generic status page responsive without waking rpcd every 5s. */
			poll.add(this._statusPollFn, 10);
			return rv;
		}, this));
	},

	unload: function() {
		this._statusStopped = true;
		if (this._statusPollFn) poll.remove(this._statusPollFn);
		this._statusPollFn = null;

		if (this._deferTimer)
			clearTimeout(this._deferTimer);

		if (this._deferIdle != null && typeof window.cancelIdleCallback === 'function')
			window.cancelIdleCallback(this._deferIdle);

		this._deferTimer = null;
		this._deferIdle = null;
	},

	handleSaveApply: null,
	handleSave: null,
	handleReset: null
});
