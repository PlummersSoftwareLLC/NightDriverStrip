'use client';

import { useThemeProps as systemUseThemeProps } from '@mui/system';
import defaultTheme from './defaultTheme';
import THEME_ID from './identifier';
export default function useThemeProps(_ref) {
  var props = _ref.props,
    name = _ref.name;
  return systemUseThemeProps({
    props: props,
    name: name,
    defaultTheme: defaultTheme,
    themeId: THEME_ID
  });
}