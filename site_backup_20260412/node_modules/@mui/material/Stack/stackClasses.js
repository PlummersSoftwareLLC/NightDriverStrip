import { unstable_generateUtilityClasses as generateUtilityClasses } from '@mui/utils';
import generateUtilityClass from '../generateUtilityClass';
export function getStackUtilityClass(slot) {
  return generateUtilityClass('MuiStack', slot);
}
const stackClasses = generateUtilityClasses('MuiStack', ['root']);
export default stackClasses;