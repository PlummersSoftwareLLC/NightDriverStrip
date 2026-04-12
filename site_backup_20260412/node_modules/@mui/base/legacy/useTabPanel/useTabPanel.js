'use client';

import * as React from 'react';
import { unstable_useId as useId, unstable_useForkRef as useForkRef } from '@mui/utils';
import { useTabsContext } from '../Tabs';
import { useCompoundItem } from '../utils/useCompoundItem';
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
  var valueParam = parameters.value,
    idParam = parameters.id,
    externalRef = parameters.rootRef;
  var context = useTabsContext();
  if (context === null) {
    throw new Error('No TabContext provided');
  }
  var selectedTabValue = context.value,
    getTabId = context.getTabId;
  var id = useId(idParam);
  var ref = React.useRef(null);
  var handleRef = useForkRef(ref, externalRef);
  var metadata = React.useMemo(function () {
    return {
      id: id,
      ref: ref
    };
  }, [id]);
  var _useCompoundItem = useCompoundItem(valueParam != null ? valueParam : tabPanelValueGenerator, metadata),
    value = _useCompoundItem.id;
  var hidden = value !== selectedTabValue;
  var correspondingTabId = value !== undefined ? getTabId(value) : undefined;
  var getRootProps = function getRootProps() {
    return {
      'aria-labelledby': correspondingTabId != null ? correspondingTabId : undefined,
      hidden: hidden,
      id: id != null ? id : undefined,
      ref: handleRef
    };
  };
  return {
    hidden: hidden,
    getRootProps: getRootProps,
    rootRef: handleRef
  };
}
export { useTabPanel };