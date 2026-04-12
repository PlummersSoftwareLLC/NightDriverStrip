"use strict";

Object.defineProperty(exports, "__esModule", {
  value: true
});
exports.getMenuButtonUtilityClass = getMenuButtonUtilityClass;
exports.menuButtonClasses = void 0;
var _generateUtilityClass = require("../generateUtilityClass");
var _generateUtilityClasses = require("../generateUtilityClasses");
function getMenuButtonUtilityClass(slot) {
  return (0, _generateUtilityClass.generateUtilityClass)('MuiMenuButton', slot);
}
const menuButtonClasses = (0, _generateUtilityClasses.generateUtilityClasses)('MuiMenuButton', ['root', 'active', 'disabled', 'expanded']);
exports.menuButtonClasses = menuButtonClasses;