'use client';

import _extends from "@babel/runtime/helpers/esm/extends";
import _slicedToArray from "@babel/runtime/helpers/esm/slicedToArray";
import * as React from 'react';
import { unstable_useControlled as useControlled } from '@mui/utils';
import { useCompoundParent } from '../utils/useCompound';
/**
 *
 * Demos:
 *
 * - [Tabs](https://mui.com/base-ui/react-tabs/#hooks)
 *
 * API:
 *
 * - [useTabs API](https://mui.com/base-ui/react-tabs/hooks-api/#use-tabs)
 */
function useTabs(parameters) {
  var valueProp = parameters.value,
    defaultValue = parameters.defaultValue,
    onChange = parameters.onChange,
    orientation = parameters.orientation,
    direction = parameters.direction,
    selectionFollowsFocus = parameters.selectionFollowsFocus;
  var _useControlled = useControlled({
      controlled: valueProp,
      default: defaultValue,
      name: 'Tabs',
      state: 'value'
    }),
    _useControlled2 = _slicedToArray(_useControlled, 2),
    value = _useControlled2[0],
    setValue = _useControlled2[1];
  var onSelected = React.useCallback(function (event, newValue) {
    setValue(newValue);
    onChange == null || onChange(event, newValue);
  }, [onChange, setValue]);
  var _useCompoundParent = useCompoundParent(),
    tabPanels = _useCompoundParent.subitems,
    compoundComponentContextValue = _useCompoundParent.contextValue;
  var tabIdLookup = React.useRef(function () {
    return undefined;
  });
  var getTabPanelId = React.useCallback(function (tabValue) {
    var _tabPanels$get;
    return (_tabPanels$get = tabPanels.get(tabValue)) == null ? void 0 : _tabPanels$get.id;
  }, [tabPanels]);
  var getTabId = React.useCallback(function (tabPanelId) {
    return tabIdLookup.current(tabPanelId);
  }, []);
  var registerTabIdLookup = React.useCallback(function (lookupFunction) {
    tabIdLookup.current = lookupFunction;
  }, []);
  return {
    contextValue: _extends({
      direction: direction,
      getTabId: getTabId,
      getTabPanelId: getTabPanelId,
      onSelected: onSelected,
      orientation: orientation,
      registerTabIdLookup: registerTabIdLookup,
      selectionFollowsFocus: selectionFollowsFocus,
      value: value
    }, compoundComponentContextValue)
  };
}
export { useTabs };