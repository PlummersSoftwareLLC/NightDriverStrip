import { generateUtilityClasses } from '../generateUtilityClasses';
import { generateUtilityClass } from '../generateUtilityClass';
export function getModalUtilityClass(slot) {
  return generateUtilityClass('MuiModal', slot);
}
export const modalClasses = generateUtilityClasses('MuiModal', ['root', 'hidden', 'backdrop']);