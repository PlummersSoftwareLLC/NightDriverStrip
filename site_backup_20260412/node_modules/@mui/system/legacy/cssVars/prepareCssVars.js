import _extends from "@babel/runtime/helpers/esm/extends";
import _slicedToArray from "@babel/runtime/helpers/esm/slicedToArray";
import _objectWithoutProperties from "@babel/runtime/helpers/esm/objectWithoutProperties";
import { deepmerge } from '@mui/utils';
import cssVarsParser from './cssVarsParser';
function prepareCssVars(theme, parserConfig) {
  // @ts-ignore - ignore components do not exist
  var _theme$colorSchemes = theme.colorSchemes,
    colorSchemes = _theme$colorSchemes === void 0 ? {} : _theme$colorSchemes,
    components = theme.components,
    otherTheme = _objectWithoutProperties(theme, ["colorSchemes", "components"]);
  var _cssVarsParser = cssVarsParser(otherTheme, parserConfig),
    rootVars = _cssVarsParser.vars,
    rootCss = _cssVarsParser.css,
    rootVarsWithDefaults = _cssVarsParser.varsWithDefaults;
  var themeVars = rootVarsWithDefaults;
  var colorSchemesMap = {};
  var light = colorSchemes.light,
    otherColorSchemes = _objectWithoutProperties(colorSchemes, ["light"]);
  Object.entries(otherColorSchemes || {}).forEach(function (_ref) {
    var _ref2 = _slicedToArray(_ref, 2),
      key = _ref2[0],
      scheme = _ref2[1];
    var _cssVarsParser2 = cssVarsParser(scheme, parserConfig),
      vars = _cssVarsParser2.vars,
      css = _cssVarsParser2.css,
      varsWithDefaults = _cssVarsParser2.varsWithDefaults;
    themeVars = deepmerge(themeVars, varsWithDefaults);
    colorSchemesMap[key] = {
      css: css,
      vars: vars
    };
  });
  if (light) {
    // light color scheme vars should be merged last to set as default
    var _cssVarsParser3 = cssVarsParser(light, parserConfig),
      css = _cssVarsParser3.css,
      vars = _cssVarsParser3.vars,
      varsWithDefaults = _cssVarsParser3.varsWithDefaults;
    themeVars = deepmerge(themeVars, varsWithDefaults);
    colorSchemesMap.light = {
      css: css,
      vars: vars
    };
  }
  var generateCssVars = function generateCssVars(colorScheme) {
    if (!colorScheme) {
      return {
        css: _extends({}, rootCss),
        vars: rootVars
      };
    }
    return {
      css: _extends({}, colorSchemesMap[colorScheme].css),
      vars: colorSchemesMap[colorScheme].vars
    };
  };
  return {
    vars: themeVars,
    generateCssVars: generateCssVars
  };
}
export default prepareCssVars;