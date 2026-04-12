"use strict";

Object.defineProperty(exports, "__esModule", {
  value: true
});
exports.ListActionTypes = void 0;
const ListActionTypes = {
  blur: 'list:blur',
  focus: 'list:focus',
  itemClick: 'list:itemClick',
  itemHover: 'list:itemHover',
  itemsChange: 'list:itemsChange',
  keyDown: 'list:keyDown',
  resetHighlight: 'list:resetHighlight',
  textNavigation: 'list:textNavigation'
};

/**
 * A union of all standard actions that can be dispatched to the list reducer.
 */
exports.ListActionTypes = ListActionTypes;