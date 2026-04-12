"use strict";

Object.defineProperty(exports, "__esModule", {
  value: true
});
exports.buttonClasses = void 0;
exports.getButtonUtilityClass = getButtonUtilityClass;
var _generateUtilityClass = require("../generateUtilityClass");
var _generateUtilityClasses = require("../generateUtilityClasses");
function getButtonUtilityClass(slot) {
  return (0, _generateUtilityClass.generateUtilityClass)('MuiButton', slot);
}
const buttonClasses = (0, _generateUtilityClasses.generateUtilityClasses)('MuiButton', ['root', 'active', 'disabled', 'focusVisible']);
exports.buttonClasses = buttonClasses;