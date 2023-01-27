const typography={
  littleHeader: {
    color: 'red'
  },
  littleValue: {
    lineHeight: 1.0,
    fontSize: "3.75rem",
    fontWeight: 100
  }
};
    
const lightTheme = createTheme({
    palette: {
      mode: 'light',
      type: 'light'
    },
    typography
});

const darkTheme = createTheme({
  palette: {
    mode: 'dark',
    type: 'dark'
  },
  typography
});
