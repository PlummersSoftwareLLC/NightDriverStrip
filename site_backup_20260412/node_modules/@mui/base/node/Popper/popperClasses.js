"use strict";

Object.defineProperty(exports, "__esModule", {
  value: true
});
exports.getPopperUtilityClass = getPopperUtilityClass;
exports.popperClasses = void 0;
var _generateUtilityClass = require("../generateUtilityClass");
var _generateUtilityClasses = require("../generateUtilityClasses");
function getPopperUtilityClass(slot) {
  return (0, _generateUtilityClass.generateUtilityClass)('MuiPopper', slot);
}
const popperClasses = (0, _generateUtilityClasses.generateUtilityClasses)('MuiPopper', ['root']);
exports.popperClasses = popperClasses;