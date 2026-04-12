'use strict';

var define = require('define-properties');
var getPolyfill = require('./polyfill');

var $IteratorPrototype = require('iterator.prototype');

module.exports = function shimIteratorPrototypeForEach() {
	var polyfill = getPolyfill();

	define(
		$IteratorPrototype,
		{ forEach: polyfill },
		{ forEach: function () { return $IteratorPrototype.forEach !== polyfill; } }
	);

	return polyfill;
};
