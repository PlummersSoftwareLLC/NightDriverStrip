"use strict";

Object.defineProperty(exports, "__esModule", {
  value: true
});
exports.getTabsListUtilityClass = getTabsListUtilityClass;
exports.tabsListClasses = void 0;
var _generateUtilityClass = require("../generateUtilityClass");
var _generateUtilityClasses = require("../generateUtilityClasses");
function getTabsListUtilityClass(slot) {
  return (0, _generateUtilityClass.generateUtilityClass)('MuiTabsList', slot);
}
const tabsListClasses = (0, _generateUtilityClasses.generateUtilityClasses)('MuiTabsList', ['root', 'horizontal', 'vertical']);
exports.tabsListClasses = tabsListClasses;