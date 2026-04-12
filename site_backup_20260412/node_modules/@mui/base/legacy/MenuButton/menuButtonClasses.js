import { generateUtilityClass } from '../generateUtilityClass';
import { generateUtilityClasses } from '../generateUtilityClasses';
export function getMenuButtonUtilityClass(slot) {
  return generateUtilityClass('MuiMenuButton', slot);
}
export var menuButtonClasses = generateUtilityClasses('MuiMenuButton', ['root', 'active', 'disabled', 'expanded']);