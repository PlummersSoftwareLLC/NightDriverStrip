"use strict";

Object.defineProperty(exports, "__esModule", {
  value: true
});
exports.getInputUtilityClass = getInputUtilityClass;
exports.inputClasses = void 0;
var _generateUtilityClass = require("../generateUtilityClass");
var _generateUtilityClasses = require("../generateUtilityClasses");
function getInputUtilityClass(slot) {
  return (0, _generateUtilityClass.generateUtilityClass)('MuiInput', slot);
}
const inputClasses = (0, _generateUtilityClasses.generateUtilityClasses)('MuiInput', ['root', 'formControl', 'focused', 'disabled', 'error', 'multiline', 'input', 'inputMultiline', 'inputTypeSearch', 'adornedStart', 'adornedEnd']);
exports.inputClasses = inputClasses;