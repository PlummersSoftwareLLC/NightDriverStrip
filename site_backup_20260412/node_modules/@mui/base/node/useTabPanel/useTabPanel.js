"use strict";
'use client';

Object.defineProperty(exports, "__esModule", {
  value: true
});
exports.useTabPanel = useTabPanel;
var React = _interopRequireWildcard(require("react"));
var _utils = require("@mui/utils");
var _Tabs = require("../Tabs");
var _useCompoundItem = require("../utils/useCompoundItem");
function _getRequireWildcardCache(nodeInterop) { if (typeof WeakMap !== "function") return null; var cacheBabelInterop = new WeakMap(); var cacheNodeInterop = new WeakMap(); return (_getRequireWildcardCache = function (nodeInterop) { return nodeInterop ? cacheNodeInterop : cacheBabelInterop; })(nodeInterop); }
function _interopRequireWildcard(obj, nodeInterop) { if (!nodeInterop && obj && obj.__esModule) { return obj; } if (obj === null || typeof obj !== "object" && typeof obj !== "function") { return { default: obj }; } var cache = _getRequireWildcardCache(nodeInterop); if (cache && cache.has(obj)) { return cache.get(obj); } var newObj = {}; var hasPropertyDescriptor = Object.defineProperty && Object.getOwnPropertyDescriptor; for (var key in obj) { if (key !== "default" && Object.prototype.hasOwnProperty.call(obj, key)) { var desc = hasPropertyDescriptor ? Object.getOwnPropertyDescriptor(obj, key) : null; if (desc && (desc.get || desc.set)) { Object.defineProperty(newObj, key, desc); } else { newObj[key] = obj[key]; } } } newObj.default = obj; if (cache) { cache.set(obj, newObj); } return newObj; }
function tabPanelValueGenerator(otherTabPanelValues) {
  return otherTabPanelValues.size;
}

/**
 *
 * Demos:
 *
 * - [Tabs](https://mui.com/base-ui/react-tabs/#hooks)
 *
 * API:
 *
 * - [useTabPanel API](https://mui.com/base-ui/react-tabs/hooks-api/#use-tab-panel)
 */
function useTabPanel(parameters) {
  const {
    value: valueParam,
    id: idParam,
    rootRef: externalRef
  } = parameters;
  const context = (0, _Tabs.useTabsContext)();
  if (context === null) {
    throw new Error('No TabContext provided');
  }
  const {
    value: selectedTabValue,
    getTabId
  } = context;
  const id = (0, _utils.unstable_useId)(idParam);
  const ref = React.useRef(null);
  const handleRef = (0, _utils.unstable_useForkRef)(ref, externalRef);
  const metadata = React.useMemo(() => ({
    id,
    ref
  }), [id]);
  const {
    id: value
  } = (0, _useCompoundItem.useCompoundItem)(valueParam != null ? valueParam : tabPanelValueGenerator, metadata);
  const hidden = value !== selectedTabValue;
  const correspondingTabId = value !== undefined ? getTabId(value) : undefined;
  const getRootProps = () => {
    return {
      'aria-labelledby': correspondingTabId != null ? correspondingTabId : undefined,
      hidden,
      id: id != null ? id : undefined,
      ref: handleRef
    };
  };
  return {
    hidden,
    getRootProps,
    rootRef: handleRef
  };
}