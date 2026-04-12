"use strict";

Object.defineProperty(exports, "__esModule", {
  value: true
});
exports.getMenuUtilityClass = getMenuUtilityClass;
exports.menuClasses = void 0;
var _generateUtilityClass = require("../generateUtilityClass");
var _generateUtilityClasses = require("../generateUtilityClasses");
function getMenuUtilityClass(slot) {
  return (0, _generateUtilityClass.generateUtilityClass)('MuiMenu', slot);
}
const menuClasses = (0, _generateUtilityClasses.generateUtilityClasses)('MuiMenu', ['root', 'listbox', 'expanded']);
exports.menuClasses = menuClasses;