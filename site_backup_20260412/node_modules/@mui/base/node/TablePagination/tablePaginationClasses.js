"use strict";

Object.defineProperty(exports, "__esModule", {
  value: true
});
exports.getTablePaginationUtilityClass = getTablePaginationUtilityClass;
exports.tablePaginationClasses = void 0;
var _generateUtilityClass = require("../generateUtilityClass");
var _generateUtilityClasses = require("../generateUtilityClasses");
function getTablePaginationUtilityClass(slot) {
  return (0, _generateUtilityClass.generateUtilityClass)('MuiTablePagination', slot);
}
const tablePaginationClasses = (0, _generateUtilityClasses.generateUtilityClasses)('MuiTablePagination', ['root', 'toolbar', 'spacer', 'selectLabel', 'selectRoot', 'select', 'selectIcon', 'input', 'menuItem', 'displayedRows', 'actions']);
exports.tablePaginationClasses = tablePaginationClasses;