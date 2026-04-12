"use strict";

Object.defineProperty(exports, "__esModule", {
  value: true
});
exports.getOptionUtilityClass = getOptionUtilityClass;
exports.optionClasses = void 0;
var _generateUtilityClass = require("../generateUtilityClass");
var _generateUtilityClasses = require("../generateUtilityClasses");
function getOptionUtilityClass(slot) {
  return (0, _generateUtilityClass.generateUtilityClass)('MuiOption', slot);
}
const optionClasses = (0, _generateUtilityClasses.generateUtilityClasses)('MuiOption', ['root', 'disabled', 'selected', 'highlighted']);
exports.optionClasses = optionClasses;