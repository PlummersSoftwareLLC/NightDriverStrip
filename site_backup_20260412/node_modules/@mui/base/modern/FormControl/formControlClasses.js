import { generateUtilityClass } from '../generateUtilityClass';
import { generateUtilityClasses } from '../generateUtilityClasses';
export function getFormControlUtilityClass(slot) {
  return generateUtilityClass('MuiFormControl', slot);
}
export const formControlClasses = generateUtilityClasses('MuiFormControl', ['root', 'disabled', 'error', 'filled', 'focused', 'required']);