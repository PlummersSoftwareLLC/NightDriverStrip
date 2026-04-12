import { generateUtilityClass } from '../generateUtilityClass';
import { generateUtilityClasses } from '../generateUtilityClasses';
export function getPopupUtilityClass(slot) {
  return generateUtilityClass('MuiPopup', slot);
}
export var popupClasses = generateUtilityClasses('MuiPopup', ['root', 'open']);