import { generateUtilityClass } from '../generateUtilityClass';
import { generateUtilityClasses } from '../generateUtilityClasses';
export function getOptionUtilityClass(slot) {
  return generateUtilityClass('MuiOption', slot);
}
export var optionClasses = generateUtilityClasses('MuiOption', ['root', 'disabled', 'selected', 'highlighted']);