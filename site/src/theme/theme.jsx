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
      type: 'light',
      taskManager: {
        strokeColor: '#90ff91',
        MemoryColor: '#0002ff',
        idleColor: 'black',
        color1: '#58be59db',
        color2: '#58be59a1',
        color3: '#58be596b',
        color4: '#58be5921',
        bcolor1: '#189cdbff',
        bcolor2: '#189cdba1',
        bcolor3: '#189cdb66',
        bcolor4: '#189cdb38',
      }
      },
    typography
});

const darkTheme = createTheme({
  palette: {
    mode: 'dark',
    type: 'dark',
    taskManager: {
      strokeColor: '#90ff91',
      MemoryColor: '#0002ff',
      idleColor: 'black',
      color1: '#58be59db',
      color2: '#58be59a1',
      color3: '#58be596b',
      color4: '#58be5921',
      bcolor1: '#189cdbff',
      bcolor2: '#189cdba1',
      bcolor3: '#189cdb66',
      bcolor4: '#189cdb38',
    }
  },
  typography
});
