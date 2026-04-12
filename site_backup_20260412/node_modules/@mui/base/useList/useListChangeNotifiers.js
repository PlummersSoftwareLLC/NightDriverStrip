'use client';

import * as React from 'react';
import { useMessageBus } from '../utils/useMessageBus';
const SELECTION_CHANGE_TOPIC = 'select:change-selection';
const HIGHLIGHT_CHANGE_TOPIC = 'select:change-highlight';
/**
 * @ignore - internal hook.
 *
 * This hook is used to notify any interested components about changes in the list's selection and highlight.
 */
export function useListChangeNotifiers() {
  const messageBus = useMessageBus();
  const notifySelectionChanged = React.useCallback(newSelectedItems => {
    messageBus.publish(SELECTION_CHANGE_TOPIC, newSelectedItems);
  }, [messageBus]);
  const notifyHighlightChanged = React.useCallback(newHighlightedItem => {
    messageBus.publish(HIGHLIGHT_CHANGE_TOPIC, newHighlightedItem);
  }, [messageBus]);
  const registerSelectionChangeHandler = React.useCallback(handler => {
    return messageBus.subscribe(SELECTION_CHANGE_TOPIC, handler);
  }, [messageBus]);
  const registerHighlightChangeHandler = React.useCallback(handler => {
    return messageBus.subscribe(HIGHLIGHT_CHANGE_TOPIC, handler);
  }, [messageBus]);
  return {
    notifySelectionChanged,
    notifyHighlightChanged,
    registerSelectionChangeHandler,
    registerHighlightChangeHandler
  };
}