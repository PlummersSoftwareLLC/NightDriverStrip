import { generateUtilityClass } from '../generateUtilityClass';
import { generateUtilityClasses } from '../generateUtilityClasses';
export function getOptionGroupUtilityClass(slot) {
  return generateUtilityClass('MuiOptionGroup', slot);
}
export const optionGroupClasses = generateUtilityClasses('MuiOptionGroup', ['root', 'disabled', 'label', 'list']);