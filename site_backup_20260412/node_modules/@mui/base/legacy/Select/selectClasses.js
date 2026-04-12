import { generateUtilityClass } from '../generateUtilityClass';
import { generateUtilityClasses } from '../generateUtilityClasses';
export function getSelectUtilityClass(slot) {
  return generateUtilityClass('MuiSelect', slot);
}
export var selectClasses = generateUtilityClasses('MuiSelect', ['root', 'button', 'listbox', 'popper', 'active', 'expanded', 'disabled', 'focusVisible']);