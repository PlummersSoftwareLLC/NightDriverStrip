import _extends from "@babel/runtime/helpers/esm/extends";
import { moveHighlight, listReducer, ListActionTypes } from '../useList';
import { SelectActionTypes } from './useSelect.types';
export function selectReducer(state, action) {
  const {
    open
  } = state;
  const {
    context: {
      selectionMode
    }
  } = action;
  if (action.type === SelectActionTypes.buttonClick) {
    const itemToHighlight = state.selectedValues[0] ?? moveHighlight(null, 'start', action.context);
    return _extends({}, state, {
      open: !open,
      highlightedValue: !open ? itemToHighlight : null
    });
  }
  const newState = listReducer(state, action);
  switch (action.type) {
    case ListActionTypes.keyDown:
      if (state.open) {
        if (action.event.key === 'Escape') {
          return _extends({}, newState, {
            open: false
          });
        }
        if (selectionMode === 'single' && (action.event.key === 'Enter' || action.event.key === ' ')) {
          return _extends({}, newState, {
            open: false
          });
        }
      } else {
        if (action.event.key === 'Enter' || action.event.key === ' ' || action.event.key === 'ArrowDown') {
          return _extends({}, state, {
            open: true,
            highlightedValue: state.selectedValues[0] ?? moveHighlight(null, 'start', action.context)
          });
        }
        if (action.event.key === 'ArrowUp') {
          return _extends({}, state, {
            open: true,
            highlightedValue: state.selectedValues[0] ?? moveHighlight(null, 'end', action.context)
          });
        }
      }
      break;
    case ListActionTypes.itemClick:
      if (selectionMode === 'single') {
        return _extends({}, newState, {
          open: false
        });
      }
      break;
    case ListActionTypes.blur:
      return _extends({}, newState, {
        open: false
      });
    default:
      return newState;
  }
  return newState;
}