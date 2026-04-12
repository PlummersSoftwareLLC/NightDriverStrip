import { generateUtilityClass } from '../generateUtilityClass';
import { generateUtilityClasses } from '../generateUtilityClasses';
export function getSwitchUtilityClass(slot) {
  return generateUtilityClass('MuiSwitch', slot);
}
export const switchClasses = generateUtilityClasses('MuiSwitch', ['root', 'input', 'track', 'thumb', 'checked', 'disabled', 'focusVisible', 'readOnly']);