'use strict';

var define = require('define-properties');
var getPolyfill = require('./polyfill');

var $IteratorPrototype = require('iterator.prototype');

module.exports = function shimIteratorPrototypeDrop() {
	var polyfill = getPolyfill();

	define(
		$IteratorPrototype,
		{ drop: polyfill },
		{ drop: function () { return $IteratorPrototype.drop !== polyfill; } }
	);

	return polyfill;
};
