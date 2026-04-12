"use strict";

Object.defineProperty(exports, "__esModule", {
  value: true
});
exports.getNumberInputUtilityClass = getNumberInputUtilityClass;
exports.numberInputClasses = void 0;
var _generateUtilityClass = require("../generateUtilityClass");
var _generateUtilityClasses = require("../generateUtilityClasses");
function getNumberInputUtilityClass(slot) {
  return (0, _generateUtilityClass.generateUtilityClass)('MuiNumberInput', slot);
}
const numberInputClasses = (0, _generateUtilityClasses.generateUtilityClasses)('MuiNumberInput', ['root', 'formControl', 'focused', 'disabled', 'readOnly', 'error', 'input', 'incrementButton', 'decrementButton'
// 'adornedStart',
// 'adornedEnd',
]);
exports.numberInputClasses = numberInputClasses;