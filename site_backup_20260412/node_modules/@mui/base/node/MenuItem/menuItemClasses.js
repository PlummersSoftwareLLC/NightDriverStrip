"use strict";

Object.defineProperty(exports, "__esModule", {
  value: true
});
exports.getMenuItemUtilityClass = getMenuItemUtilityClass;
exports.menuItemClasses = void 0;
var _generateUtilityClass = require("../generateUtilityClass");
var _generateUtilityClasses = require("../generateUtilityClasses");
function getMenuItemUtilityClass(slot) {
  return (0, _generateUtilityClass.generateUtilityClass)('MuiMenuItem', slot);
}
const menuItemClasses = (0, _generateUtilityClasses.generateUtilityClasses)('MuiMenuItem', ['root', 'disabled', 'focusVisible']);
exports.menuItemClasses = menuItemClasses;