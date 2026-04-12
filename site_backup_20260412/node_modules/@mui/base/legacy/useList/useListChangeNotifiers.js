'use client';

import * as React from 'react';
import { useMessageBus } from '../utils/useMessageBus';
var SELECTION_CHANGE_TOPIC = 'select:change-selection';
var HIGHLIGHT_CHANGE_TOPIC = 'select:change-highlight';
/**
 * @ignore - internal hook.
 *
 * This hook is used to notify any interested components about changes in the list's selection and highlight.
 */
export function useListChangeNotifiers() {
  var messageBus = useMessageBus();
  var notifySelectionChanged = React.useCallback(function (newSelectedItems) {
    messageBus.publish(SELECTION_CHANGE_TOPIC, newSelectedItems);
  }, [messageBus]);
  var notifyHighlightChanged = React.useCallback(function (newHighlightedItem) {
    messageBus.publish(HIGHLIGHT_CHANGE_TOPIC, newHighlightedItem);
  }, [messageBus]);
  var registerSelectionChangeHandler = React.useCallback(function (handler) {
    return messageBus.subscribe(SELECTION_CHANGE_TOPIC, handler);
  }, [messageBus]);
  var registerHighlightChangeHandler = React.useCallback(function (handler) {
    return messageBus.subscribe(HIGHLIGHT_CHANGE_TOPIC, handler);
  }, [messageBus]);
  return {
    notifySelectionChanged: notifySelectionChanged,
    notifyHighlightChanged: notifyHighlightChanged,
    registerSelectionChangeHandler: registerSelectionChangeHandler,
    registerHighlightChangeHandler: registerHighlightChangeHandler
  };
}