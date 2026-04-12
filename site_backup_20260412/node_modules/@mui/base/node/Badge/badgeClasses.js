"use strict";

Object.defineProperty(exports, "__esModule", {
  value: true
});
exports.badgeClasses = void 0;
exports.getBadgeUtilityClass = getBadgeUtilityClass;
var _generateUtilityClasses = require("../generateUtilityClasses");
var _generateUtilityClass = require("../generateUtilityClass");
function getBadgeUtilityClass(slot) {
  return (0, _generateUtilityClass.generateUtilityClass)('MuiBadge', slot);
}
const badgeClasses = (0, _generateUtilityClasses.generateUtilityClasses)('MuiBadge', ['root', 'badge', 'invisible']);
exports.badgeClasses = badgeClasses;