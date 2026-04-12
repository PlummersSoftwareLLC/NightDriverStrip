"use strict";

var _interopRequireDefault = require("@babel/runtime/helpers/interopRequireDefault");
Object.defineProperty(exports, "__esModule", {
  value: true
});
exports.default = void 0;
exports.getModalUtilityClass = getModalUtilityClass;
var _utils = require("@mui/utils");
var _generateUtilityClass = _interopRequireDefault(require("../generateUtilityClass"));
function getModalUtilityClass(slot) {
  return (0, _generateUtilityClass.default)('MuiModal', slot);
}
const modalClasses = (0, _utils.unstable_generateUtilityClasses)('MuiModal', ['root', 'hidden', 'backdrop']);
var _default = modalClasses;
exports.default = _default;