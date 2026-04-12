'use client';

import _extends from "@babel/runtime/helpers/esm/extends";
import * as React from 'react';
import { unstable_useForkRef as useForkRef, unstable_useId as useId } from '@mui/utils';
import { useListItem } from '../useList';
import { useCompoundItem } from '../utils/useCompoundItem';

/**
 *
 * Demos:
 *
 * - [Select](https://mui.com/base-ui/react-select/#hooks)
 *
 * API:
 *
 * - [useOption API](https://mui.com/base-ui/react-select/hooks-api/#use-option)
 */
export function useOption(params) {
  var value = params.value,
    label = params.label,
    disabled = params.disabled,
    optionRefParam = params.rootRef,
    idParam = params.id;
  var _useListItem = useListItem({
      item: value
    }),
    getListItemProps = _useListItem.getRootProps,
    listItemRefHandler = _useListItem.rootRef,
    highlighted = _useListItem.highlighted,
    selected = _useListItem.selected;
  var id = useId(idParam);
  var optionRef = React.useRef(null);
  var selectOption = React.useMemo(function () {
    return {
      disabled: disabled,
      label: label,
      value: value,
      ref: optionRef,
      id: id
    };
  }, [disabled, label, value, id]);
  var _useCompoundItem = useCompoundItem(value, selectOption),
    index = _useCompoundItem.index;
  var handleRef = useForkRef(optionRefParam, optionRef, listItemRefHandler);
  return {
    getRootProps: function getRootProps() {
      var otherHandlers = arguments.length > 0 && arguments[0] !== undefined ? arguments[0] : {};
      return _extends({}, otherHandlers, getListItemProps(otherHandlers), {
        id: id,
        ref: handleRef,
        role: 'option',
        'aria-selected': selected
      });
    },
    highlighted: highlighted,
    index: index,
    selected: selected,
    rootRef: handleRef
  };
}