import { generateUtilityClass } from '../generateUtilityClass';
import { generateUtilityClasses } from '../generateUtilityClasses';
export function getPopperUtilityClass(slot) {
  return generateUtilityClass('MuiPopper', slot);
}
export var popperClasses = generateUtilityClasses('MuiPopper', ['root']);