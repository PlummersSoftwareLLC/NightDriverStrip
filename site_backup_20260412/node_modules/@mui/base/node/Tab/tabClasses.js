"use strict";

Object.defineProperty(exports, "__esModule", {
  value: true
});
exports.getTabUtilityClass = getTabUtilityClass;
exports.tabClasses = void 0;
var _generateUtilityClass = require("../generateUtilityClass");
var _generateUtilityClasses = require("../generateUtilityClasses");
function getTabUtilityClass(slot) {
  return (0, _generateUtilityClass.generateUtilityClass)('MuiTab', slot);
}
const tabClasses = (0, _generateUtilityClasses.generateUtilityClasses)('MuiTab', ['root', 'selected', 'disabled']);
exports.tabClasses = tabClasses;