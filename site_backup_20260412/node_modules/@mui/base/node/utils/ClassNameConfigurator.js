"use strict";
'use client';

Object.defineProperty(exports, "__esModule", {
  value: true
});
exports.ClassNameConfigurator = ClassNameConfigurator;
exports.useClassNamesOverride = useClassNamesOverride;
var React = _interopRequireWildcard(require("react"));
var _jsxRuntime = require("react/jsx-runtime");
function _getRequireWildcardCache(nodeInterop) { if (typeof WeakMap !== "function") return null; var cacheBabelInterop = new WeakMap(); var cacheNodeInterop = new WeakMap(); return (_getRequireWildcardCache = function (nodeInterop) { return nodeInterop ? cacheNodeInterop : cacheBabelInterop; })(nodeInterop); }
function _interopRequireWildcard(obj, nodeInterop) { if (!nodeInterop && obj && obj.__esModule) { return obj; } if (obj === null || typeof obj !== "object" && typeof obj !== "function") { return { default: obj }; } var cache = _getRequireWildcardCache(nodeInterop); if (cache && cache.has(obj)) { return cache.get(obj); } var newObj = {}; var hasPropertyDescriptor = Object.defineProperty && Object.getOwnPropertyDescriptor; for (var key in obj) { if (key !== "default" && Object.prototype.hasOwnProperty.call(obj, key)) { var desc = hasPropertyDescriptor ? Object.getOwnPropertyDescriptor(obj, key) : null; if (desc && (desc.get || desc.set)) { Object.defineProperty(newObj, key, desc); } else { newObj[key] = obj[key]; } } } newObj.default = obj; if (cache) { cache.set(obj, newObj); } return newObj; }
const defaultContextValue = {
  disableDefaultClasses: false
};
const ClassNameConfiguratorContext = /*#__PURE__*/React.createContext(defaultContextValue);
/**
 * @ignore - internal hook.
 *
 * Wraps the `generateUtilityClass` function and controls how the classes are generated.
 * Currently it only affects whether the classes are applied or not.
 *
 * @returns Function to be called with the `generateUtilityClass` function specific to a component to generate the classes.
 */
function useClassNamesOverride(generateUtilityClass) {
  const {
    disableDefaultClasses
  } = React.useContext(ClassNameConfiguratorContext);
  return slot => {
    if (disableDefaultClasses) {
      return '';
    }
    return generateUtilityClass(slot);
  };
}

/**
 * Allows to configure the components within to not apply any built-in classes.
 */
function ClassNameConfigurator(props) {
  const {
    disableDefaultClasses,
    children
  } = props;
  const contextValue = React.useMemo(() => ({
    disableDefaultClasses: disableDefaultClasses != null ? disableDefaultClasses : false
  }), [disableDefaultClasses]);
  return /*#__PURE__*/(0, _jsxRuntime.jsx)(ClassNameConfiguratorContext.Provider, {
    value: contextValue,
    children: children
  });
}