import { generateUtilityClass } from '../generateUtilityClass';
import { generateUtilityClasses } from '../generateUtilityClasses';
export function getMenuItemUtilityClass(slot) {
  return generateUtilityClass('MuiMenuItem', slot);
}
export const menuItemClasses = generateUtilityClasses('MuiMenuItem', ['root', 'disabled', 'focusVisible']);