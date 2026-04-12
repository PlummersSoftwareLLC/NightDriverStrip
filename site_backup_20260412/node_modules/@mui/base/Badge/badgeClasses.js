import { generateUtilityClasses } from '../generateUtilityClasses';
import { generateUtilityClass } from '../generateUtilityClass';
export function getBadgeUtilityClass(slot) {
  return generateUtilityClass('MuiBadge', slot);
}
export const badgeClasses = generateUtilityClasses('MuiBadge', ['root', 'badge', 'invisible']);