'use strict';

var define = require('define-properties');
var getPolyfill = require('./polyfill');

var $IteratorPrototype = require('iterator.prototype');

module.exports = function shimIteratorPrototypeFind() {
	var polyfill = getPolyfill();

	define(
		$IteratorPrototype,
		{ find: polyfill },
		{ find: function () { return $IteratorPrototype.find !== polyfill; } }
	);

	return polyfill;
};
