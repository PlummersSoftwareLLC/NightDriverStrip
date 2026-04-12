"use strict";

Object.defineProperty(exports, "__esModule", {
  value: true
});
exports.getModalUtilityClass = getModalUtilityClass;
exports.modalClasses = void 0;
var _generateUtilityClasses = require("../generateUtilityClasses");
var _generateUtilityClass = require("../generateUtilityClass");
function getModalUtilityClass(slot) {
  return (0, _generateUtilityClass.generateUtilityClass)('MuiModal', slot);
}
const modalClasses = (0, _generateUtilityClasses.generateUtilityClasses)('MuiModal', ['root', 'hidden', 'backdrop']);
exports.modalClasses = modalClasses;