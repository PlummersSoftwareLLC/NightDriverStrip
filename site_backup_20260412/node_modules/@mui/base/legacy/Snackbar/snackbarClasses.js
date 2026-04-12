import { generateUtilityClass } from '../generateUtilityClass';
import { generateUtilityClasses } from '../generateUtilityClasses';
export function getSnackbarUtilityClass(slot) {
  return generateUtilityClass('MuiSnackbar', slot);
}
export var snackbarClasses = generateUtilityClasses('MuiSnackbar', ['root']);