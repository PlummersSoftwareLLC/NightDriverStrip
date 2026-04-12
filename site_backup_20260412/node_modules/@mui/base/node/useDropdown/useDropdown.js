"use strict";
'use client';

Object.defineProperty(exports, "__esModule", {
  value: true
});
exports.useDropdown = useDropdown;
var React = _interopRequireWildcard(require("react"));
var _useControllableReducer = require("../utils/useControllableReducer");
var _useDropdown = require("./useDropdown.types");
var _dropdownReducer = require("./dropdownReducer");
function _getRequireWildcardCache(nodeInterop) { if (typeof WeakMap !== "function") return null; var cacheBabelInterop = new WeakMap(); var cacheNodeInterop = new WeakMap(); return (_getRequireWildcardCache = function (nodeInterop) { return nodeInterop ? cacheNodeInterop : cacheBabelInterop; })(nodeInterop); }
function _interopRequireWildcard(obj, nodeInterop) { if (!nodeInterop && obj && obj.__esModule) { return obj; } if (obj === null || typeof obj !== "object" && typeof obj !== "function") { return { default: obj }; } var cache = _getRequireWildcardCache(nodeInterop); if (cache && cache.has(obj)) { return cache.get(obj); } var newObj = {}; var hasPropertyDescriptor = Object.defineProperty && Object.getOwnPropertyDescriptor; for (var key in obj) { if (key !== "default" && Object.prototype.hasOwnProperty.call(obj, key)) { var desc = hasPropertyDescriptor ? Object.getOwnPropertyDescriptor(obj, key) : null; if (desc && (desc.get || desc.set)) { Object.defineProperty(newObj, key, desc); } else { newObj[key] = obj[key]; } } } newObj.default = obj; if (cache) { cache.set(obj, newObj); } return newObj; }
/**
 *
 * Demos:
 *
 * - [Menu](https://mui.com/base-ui/react-menu/#hooks)
 *
 * API:
 *
 * - [useDropdown API](https://mui.com/base-ui/react-menu/hooks-api/#use-dropdown)
 */
function useDropdown(parameters = {}) {
  const {
    defaultOpen,
    onOpenChange,
    open: openProp
  } = parameters;
  const [popupId, setPopupId] = React.useState('');
  const [triggerElement, setTriggerElement] = React.useState(null);
  const lastActionType = React.useRef(null);
  const handleStateChange = React.useCallback((event, field, value, reason) => {
    if (field === 'open') {
      onOpenChange == null || onOpenChange(event, value);
    }
    lastActionType.current = reason;
  }, [onOpenChange]);
  const controlledProps = React.useMemo(() => openProp !== undefined ? {
    open: openProp
  } : {}, [openProp]);
  const [state, dispatch] = (0, _useControllableReducer.useControllableReducer)({
    controlledProps,
    initialState: defaultOpen ? {
      open: true
    } : {
      open: false
    },
    onStateChange: handleStateChange,
    reducer: _dropdownReducer.dropdownReducer
  });
  React.useEffect(() => {
    if (!state.open && lastActionType.current !== null && lastActionType.current !== _useDropdown.DropdownActionTypes.blur) {
      triggerElement == null || triggerElement.focus();
    }
  }, [state.open, triggerElement]);
  const contextValue = {
    state,
    dispatch,
    popupId,
    registerPopup: setPopupId,
    registerTrigger: setTriggerElement,
    triggerElement
  };
  return {
    contextValue,
    open: state.open
  };
}