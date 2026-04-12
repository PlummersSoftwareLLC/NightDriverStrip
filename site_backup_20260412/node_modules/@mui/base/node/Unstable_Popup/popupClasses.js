"use strict";

Object.defineProperty(exports, "__esModule", {
  value: true
});
exports.getPopupUtilityClass = getPopupUtilityClass;
exports.popupClasses = void 0;
var _generateUtilityClass = require("../generateUtilityClass");
var _generateUtilityClasses = require("../generateUtilityClasses");
function getPopupUtilityClass(slot) {
  return (0, _generateUtilityClass.generateUtilityClass)('MuiPopup', slot);
}
const popupClasses = (0, _generateUtilityClasses.generateUtilityClasses)('MuiPopup', ['root', 'open']);
exports.popupClasses = popupClasses;