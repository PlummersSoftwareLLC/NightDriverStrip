import { generateUtilityClass } from '../generateUtilityClass';
import { generateUtilityClasses } from '../generateUtilityClasses';
export function getTabUtilityClass(slot) {
  return generateUtilityClass('MuiTab', slot);
}
export const tabClasses = generateUtilityClasses('MuiTab', ['root', 'selected', 'disabled']);