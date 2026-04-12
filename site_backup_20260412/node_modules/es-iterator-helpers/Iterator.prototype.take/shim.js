'use strict';

var define = require('define-properties');
var getPolyfill = require('./polyfill');

var $IteratorPrototype = require('iterator.prototype');

module.exports = function shimIteratorPrototypeTake() {
	var polyfill = getPolyfill();

	define(
		$IteratorPrototype,
		{ take: polyfill },
		{ take: function () { return $IteratorPrototype.take !== polyfill; } }
	);

	return polyfill;
};
