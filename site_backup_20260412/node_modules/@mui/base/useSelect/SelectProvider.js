'use client';

import * as React from 'react';
import { ListContext } from '../useList/ListContext';
import { CompoundComponentContext } from '../utils/useCompound';
import { jsx as _jsx } from "react/jsx-runtime";
/**
 * Sets up the contexts for the underlying Option components.
 *
 * @ignore - do not document.
 */
export function SelectProvider(props) {
  const {
    value,
    children
  } = props;
  const {
    dispatch,
    getItemIndex,
    getItemState,
    registerHighlightChangeHandler,
    registerSelectionChangeHandler,
    registerItem,
    totalSubitemCount
  } = value;
  const listContextValue = React.useMemo(() => ({
    dispatch,
    getItemState,
    getItemIndex,
    registerHighlightChangeHandler,
    registerSelectionChangeHandler
  }), [dispatch, getItemIndex, getItemState, registerHighlightChangeHandler, registerSelectionChangeHandler]);
  const compoundComponentContextValue = React.useMemo(() => ({
    getItemIndex,
    registerItem,
    totalSubitemCount
  }), [registerItem, getItemIndex, totalSubitemCount]);
  return /*#__PURE__*/_jsx(CompoundComponentContext.Provider, {
    value: compoundComponentContextValue,
    children: /*#__PURE__*/_jsx(ListContext.Provider, {
      value: listContextValue,
      children: children
    })
  });
}