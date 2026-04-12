"use strict";

Object.defineProperty(exports, "__esModule", {
  value: true
});
exports.dropdownReducer = dropdownReducer;
var _useDropdown = require("./useDropdown.types");
function dropdownReducer(state, action) {
  switch (action.type) {
    case _useDropdown.DropdownActionTypes.blur:
      return {
        open: false
      };
    case _useDropdown.DropdownActionTypes.escapeKeyDown:
      return {
        open: false
      };
    case _useDropdown.DropdownActionTypes.toggle:
      return {
        open: !state.open
      };
    case _useDropdown.DropdownActionTypes.open:
      return {
        open: true
      };
    case _useDropdown.DropdownActionTypes.close:
      return {
        open: false
      };
    default:
      throw new Error(`Unhandled action`);
  }
}