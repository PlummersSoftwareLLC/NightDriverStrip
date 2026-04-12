"use strict";

var _interopRequireDefault = require("@babel/runtime/helpers/interopRequireDefault");
Object.defineProperty(exports, "__esModule", {
  value: true
});
exports.default = void 0;
exports.getSliderUtilityClass = getSliderUtilityClass;
var _utils = require("@mui/utils");
var _generateUtilityClass = _interopRequireDefault(require("../generateUtilityClass"));
function getSliderUtilityClass(slot) {
  return (0, _generateUtilityClass.default)('MuiSlider', slot);
}
const sliderClasses = (0, _utils.unstable_generateUtilityClasses)('MuiSlider', ['root', 'active', 'colorPrimary', 'colorSecondary', 'disabled', 'dragging', 'focusVisible', 'mark', 'markActive', 'marked', 'markLabel', 'markLabelActive', 'rail', 'sizeSmall', 'thumb', 'thumbColorPrimary', 'thumbColorSecondary', 'track', 'trackInverted', 'trackFalse', 'thumbSizeSmall', 'valueLabel', 'valueLabelOpen', 'valueLabelCircle', 'valueLabelLabel', 'vertical']);
var _default = sliderClasses;
exports.default = _default;