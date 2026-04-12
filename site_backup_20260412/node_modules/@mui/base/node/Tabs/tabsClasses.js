"use strict";

Object.defineProperty(exports, "__esModule", {
  value: true
});
exports.getTabsUtilityClass = getTabsUtilityClass;
exports.tabsClasses = void 0;
var _generateUtilityClass = require("../generateUtilityClass");
var _generateUtilityClasses = require("../generateUtilityClasses");
function getTabsUtilityClass(slot) {
  return (0, _generateUtilityClass.generateUtilityClass)('MuiTabs', slot);
}
const tabsClasses = (0, _generateUtilityClasses.generateUtilityClasses)('MuiTabs', ['root', 'horizontal', 'vertical']);
exports.tabsClasses = tabsClasses;