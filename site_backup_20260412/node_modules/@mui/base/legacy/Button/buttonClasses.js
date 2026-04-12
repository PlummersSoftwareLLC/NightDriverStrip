import { generateUtilityClass } from '../generateUtilityClass';
import { generateUtilityClasses } from '../generateUtilityClasses';
export function getButtonUtilityClass(slot) {
  return generateUtilityClass('MuiButton', slot);
}
export var buttonClasses = generateUtilityClasses('MuiButton', ['root', 'active', 'disabled', 'focusVisible']);