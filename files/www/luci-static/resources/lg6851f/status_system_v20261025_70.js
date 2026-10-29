'use strict';
'require baseclass';
'require rpc';

var callSystemInfo = rpc.declare({ object: 'system', method: 'info' });
var value = null;
var valueAt = 0;
var flight = null;
var maxAge = 750;

function get() {
	var now = Date.now();

	if (value && now - valueAt <= maxAge)
		return Promise.resolve(value);

	if (flight)
		return flight;

	flight = callSystemInfo().then(function(reply) {
		value = reply;
		valueAt = Date.now();
		return reply;
	}).finally(function() {
		flight = null;
	});

	return flight;
}

return baseclass.extend({ get: get });
