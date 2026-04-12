"use strict";

Object.defineProperty(exports, "__esModule", {
  value: true
});
exports.getOptionGroupUtilityClass = getOptionGroupUtilityClass;
exports.optionGroupClasses = void 0;
var _generateUtilityClass = require("../generateUtilityClass");
var _generateUtilityClasses = require("../generateUtilityClasses");
function getOptionGroupUtilityClass(slot) {
  return (0, _generateUtilityClass.generateUtilityClass)('MuiOptionGroup', slot);
}
const optionGroupClasses = (0, _generateUtilityClasses.generateUtilityClasses)('MuiOptionGroup', ['root', 'disabled', 'label', 'list']);
exports.optionGroupClasses = optionGroupClasses;