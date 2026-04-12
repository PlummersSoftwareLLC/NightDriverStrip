"use strict";

Object.defineProperty(exports, "__esModule", {
  value: true
});
exports.getSnackbarUtilityClass = getSnackbarUtilityClass;
exports.snackbarClasses = void 0;
var _generateUtilityClass = require("../generateUtilityClass");
var _generateUtilityClasses = require("../generateUtilityClasses");
function getSnackbarUtilityClass(slot) {
  return (0, _generateUtilityClass.generateUtilityClass)('MuiSnackbar', slot);
}
const snackbarClasses = (0, _generateUtilityClasses.generateUtilityClasses)('MuiSnackbar', ['root']);
exports.snackbarClasses = snackbarClasses;