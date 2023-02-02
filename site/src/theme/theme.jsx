const pannelText= {
  fontFamily: ["Roboto", "Helvetica", "Arial", "sans-serif"].join(", ")
};

const commonTypography={
  littleHeader: {
      fontWeight: 500,
      fontSize: "1.25rem",
      lineHeight: 1.6,
      letterSpacing: "0.0075em",
  },
  littleValue: {
    lineHeight: 1.0,
    fontSize: "3.75rem",
    fontWeight: 300
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
  typography: commonTypography,
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
    },
    text: {
      primary: "#97ea44",
      secondary: "aquamarine",
      attribute: "aqua",
      icon: "aquamarine"
    },
    primary: {
      main: "#97ea44"
    }
  },
  typography: commonTypography,
  overrides: {
    MuiAppBar: {
      colorPrimary: {
        backgroundColor: "black",
        color: "textPrimary"
      }
    },
    MuiIconButton: {
      colorPrimary: {
        color: "aquamarine"
      },
      colorSecondary: {
        color: "aqua"
      },
      root:{
        color: "lightgreen",
      }
    },
    MuiCheckbox: {
      colorPrimary: {
        color: "textPrimary"
      },
      colorSecondary: {
        "&$checked": "aquamarine"
      }
    }
  },
});
