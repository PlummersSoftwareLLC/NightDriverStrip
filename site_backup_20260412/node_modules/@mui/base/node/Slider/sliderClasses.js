"use strict";

Object.defineProperty(exports, "__esModule", {
  value: true
});
exports.getSliderUtilityClass = getSliderUtilityClass;
exports.sliderClasses = void 0;
var _generateUtilityClasses = require("../generateUtilityClasses");
var _generateUtilityClass = require("../generateUtilityClass");
function getSliderUtilityClass(slot) {
  return (0, _generateUtilityClass.generateUtilityClass)('MuiSlider', slot);
}
const sliderClasses = (0, _generateUtilityClasses.generateUtilityClasses)('MuiSlider', ['root', 'active', 'focusVisible', 'disabled', 'dragging', 'marked', 'vertical', 'trackInverted', 'trackFalse', 'rail', 'track', 'mark', 'markActive', 'markLabel', 'markLabelActive', 'thumb']);
exports.sliderClasses = sliderClasses;