'use client';

import { createStyled, shouldForwardProp } from '@mui/system';
import defaultTheme from './defaultTheme';
import THEME_ID from './identifier';
export var rootShouldForwardProp = function rootShouldForwardProp(prop) {
  return shouldForwardProp(prop) && prop !== 'classes';
};
export var slotShouldForwardProp = shouldForwardProp;
var styled = createStyled({
  themeId: THEME_ID,
  defaultTheme: defaultTheme,
  rootShouldForwardProp: rootShouldForwardProp
});
export default styled;