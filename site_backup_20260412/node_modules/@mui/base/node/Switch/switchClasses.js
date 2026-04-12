"use strict";

Object.defineProperty(exports, "__esModule", {
  value: true
});
exports.getSwitchUtilityClass = getSwitchUtilityClass;
exports.switchClasses = void 0;
var _generateUtilityClass = require("../generateUtilityClass");
var _generateUtilityClasses = require("../generateUtilityClasses");
function getSwitchUtilityClass(slot) {
  return (0, _generateUtilityClass.generateUtilityClass)('MuiSwitch', slot);
}
const switchClasses = (0, _generateUtilityClasses.generateUtilityClasses)('MuiSwitch', ['root', 'input', 'track', 'thumb', 'checked', 'disabled', 'focusVisible', 'readOnly']);
exports.switchClasses = switchClasses;