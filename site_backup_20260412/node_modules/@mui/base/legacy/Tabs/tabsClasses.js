import { generateUtilityClass } from '../generateUtilityClass';
import { generateUtilityClasses } from '../generateUtilityClasses';
export function getTabsUtilityClass(slot) {
  return generateUtilityClass('MuiTabs', slot);
}
export var tabsClasses = generateUtilityClasses('MuiTabs', ['root', 'horizontal', 'vertical']);