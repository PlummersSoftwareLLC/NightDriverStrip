import { generateUtilityClass } from '../generateUtilityClass';
import { generateUtilityClasses } from '../generateUtilityClasses';
export function getTabsListUtilityClass(slot) {
  return generateUtilityClass('MuiTabsList', slot);
}
export var tabsListClasses = generateUtilityClasses('MuiTabsList', ['root', 'horizontal', 'vertical']);