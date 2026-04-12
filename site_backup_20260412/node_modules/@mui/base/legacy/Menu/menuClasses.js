import { generateUtilityClass } from '../generateUtilityClass';
import { generateUtilityClasses } from '../generateUtilityClasses';
export function getMenuUtilityClass(slot) {
  return generateUtilityClass('MuiMenu', slot);
}
export var menuClasses = generateUtilityClasses('MuiMenu', ['root', 'listbox', 'expanded']);