import { generateUtilityClass } from '../generateUtilityClass';
import { generateUtilityClasses } from '../generateUtilityClasses';
export function getPopupUtilityClass(slot) {
  return generateUtilityClass('MuiPopup', slot);
}
export const popupClasses = generateUtilityClasses('MuiPopup', ['root', 'open']);