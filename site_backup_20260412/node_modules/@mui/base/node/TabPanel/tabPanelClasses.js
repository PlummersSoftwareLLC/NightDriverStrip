"use strict";

Object.defineProperty(exports, "__esModule", {
  value: true
});
exports.getTabPanelUtilityClass = getTabPanelUtilityClass;
exports.tabPanelClasses = void 0;
var _generateUtilityClass = require("../generateUtilityClass");
var _generateUtilityClasses = require("../generateUtilityClasses");
function getTabPanelUtilityClass(slot) {
  return (0, _generateUtilityClass.generateUtilityClass)('MuiTabPanel', slot);
}
const tabPanelClasses = (0, _generateUtilityClasses.generateUtilityClasses)('MuiTabPanel', ['root', 'hidden']);
exports.tabPanelClasses = tabPanelClasses;