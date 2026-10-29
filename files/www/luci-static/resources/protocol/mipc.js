'use strict';
'require form';
'require network';

network.registerPatternVirtual(/^ccmni\d*$/);
network.registerErrorCode('MODEM_STACK_NOT_READY', _('The modem service chain is not ready.'));
network.registerErrorCode('MODEM_EXCEPTION', _('The modem is in an exception state.'));
network.registerErrorCode('SIM_NOT_INITIALIZED', _('The SIM card is not ready.'));
network.registerErrorCode('CONNECT_FAILED_RETRY_LATER', _('The mobile data call failed.'));

return network.registerProtocol('mipc', {
	getI18n: function() {
		return _('MT6990 cellular');
	},

	getIfname: function() {
		return this._ubus('l3_device') || 'ccmni';
	},

	getPackageName: function() {
		return 'mipc-wan';
	},

	isFloating: function() {
		return true;
	},

	isVirtual: function() {
		return true;
	},

	getDevices: function() {
		return null;
	},

	containsDevice: function(ifname) {
		return network.getIfnameOf(ifname) == this.getIfname();
	},

	renderFormOptions: function(s) {
		var o;

		o = s.taboption('general', form.Value, 'apn', _('APN'));
		o.placeholder = '3gnet';

		o = s.taboption('general', form.ListValue, 'iptype', _('IP type'));
		o.value('1', _('IPv4'));
		o.value('2', _('IPv6'));
		o.value('3', _('IPv4/IPv6'));
		o.default = '3';

		o = s.taboption('advanced', form.Value, 'mtu', _('Override MTU'));
		o.datatype = 'range(1280,1500)';
		o.placeholder = '1400';
	}
});
