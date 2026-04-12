"use strict";

Object.defineProperty(exports, "__esModule", {
  value: true
});
exports.getSelectUtilityClass = getSelectUtilityClass;
exports.selectClasses = void 0;
var _generateUtilityClass = require("../generateUtilityClass");
var _generateUtilityClasses = require("../generateUtilityClasses");
function getSelectUtilityClass(slot) {
  return (0, _generateUtilityClass.generateUtilityClass)('MuiSelect', slot);
}
const selectClasses = (0, _generateUtilityClasses.generateUtilityClasses)('MuiSelect', ['root', 'button', 'listbox', 'popper', 'active', 'expanded', 'disabled', 'focusVisible']);
exports.selectClasses = selectClasses;