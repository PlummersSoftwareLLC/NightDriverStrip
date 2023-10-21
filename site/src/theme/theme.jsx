import { createTheme } from '@mui/material';

const getPalette = (mode) => {
    switch (mode) {
    case "dark":
        return {
            common: {
                black: "#000",
                white: "#fff"
            },
            a: {
                link: "#29b6f6",
                visited: "#ce93d8"
            },
            primary: {
                main: '#24292e',
                light: '#4f5358',
                dark: '#191c21',
                contrastText: '#d6d6ff',
            },
            secondary: {
                main: "#ce93d8",
                light: "#f3e5f5",
                dark: "#ab47bc",
                contrastText: "rgba(0, 0, 0, 0.87)"
            },
            error: {
                main: "#f44336",
                light: "#e57373",
                dark: "#d32f2f",
                contrastText: "#fff"
            },
            warning: {
                main: "#ffa726",
                light: "#ffb74d",
                dark: "#f57c00",
                contrastText: "rgba(0, 0, 0, 0.87)"
            },
            info: {
                main: "#29b6f6",
                light: "#4fc3f7",
                dark: "#0288d1",
                contrastText: "rgba(0, 0, 0, 0.87)"
            },
            success: {
                main: "#66bb6a",
                light: "#81c784",
                dark: "#388e3c",
                contrastText: "rgba(0, 0, 0, 0.87)"
            },
            grey: {
                50: "#fafafa",
                100: "#f5f5f5",
                200: "#eeeeee",
                300: "#e0e0e0",
                400: "#bdbdbd",
                500: "#9e9e9e",
                600: "#757575",
                700: "#616161",
                800: "#424242",
                900: "#212121",
                A100: "#f5f5f5",
                A200: "#eeeeee",
                A400: "#bdbdbd",
                A700: "#616161"
            },
            palette: {
                mode,
               
                contrastThreshold: 3,
                tonalOffset: 0.2,
                text: {
                    primary: '#93aff3',
                    secondary: 'rgba(149,183,228,0.7)',
                    disabled: "rgba(255, 255, 255, 0.5)",
                    icon: "rgba(255, 255, 255, 0.5)"
                },
                divider: "rgba(255, 255, 255, 0.12)",
                background: {
                    paper: "hwb(216deg 14% 73% / 99%)",
                    default: "hwb(215deg 8% 83%)"
                },
                action: {
                    active: "#fff",
                    hover: "rgba(255, 255, 255, 0.08)",
                    hoverOpacity: 0.08,
                    selected: "rgba(255, 255, 255, 0.16)",
                    selectedOpacity: 0.16,
                    disabled: "rgba(255, 255, 255, 0.3)",
                    disabledBackground: "rgba(255, 255, 255, 0.12)",
                    disabledOpacity: 0.38,
                    focus: "rgba(255, 255, 255, 0.12)",
                    focusOpacity: 0.12,
                    activatedOpacity: 0.24
                },
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
        };
    case "light": 
        return {
            common: {
                black: "#000",
                white: "#fff"
            },
            a: {
                link: "#0288d1",
                visited: "#9c27b0"
            },
            primary: {
                main: "#1976d2",
                light: "#42a5f5",
                dark: "#1565c0",
                contrastText: "#fff"
            },
            secondary: {
                main: "#9c27b0",
                light: "#ba68c8",
                dark: "#7b1fa2",
                contrastText: "#fff"
            },
            error: {
                main: "#d32f2f",
                light: "#ef5350",
                dark: "#c62828",
                contrastText: "#fff"
            },
            warning: {
                main: "#ed6c02",
                light: "#ff9800",
                dark: "#e65100",
                contrastText: "#fff"
            },
            info: {
                main: "#0288d1",
                light: "#03a9f4",
                dark: "#01579b",
                contrastText: "#fff"
            },
            success: {
                main: "#2e7d32",
                light: "#4caf50",
                dark: "#1b5e20",
                contrastText: "#fff"
            },
            grey: {
                50: "#fafafa",
                100: "#f5f5f5",
                200: "#eeeeee",
                300: "#e0e0e0",
                400: "#bdbdbd",
                500: "#9e9e9e",
                600: "#757575",
                700: "#616161",
                800: "#424242",
                900: "#212121",
                A100: "#f5f5f5",
                A200: "#eeeeee",
                A400: "#bdbdbd",
                A700: "#616161"
            },
            palette: {

                mode: mode,
               
                contrastThreshold: 3,
                tonalOffset: 0.2,
                text: {
                    primary: "rgba(0, 0, 0, 0.87)",
                    secondary: "rgba(0, 0, 0, 0.6)",
                    disabled: "rgba(0, 0, 0, 0.38)"
                },
                divider: "rgba(0, 0, 0, 0.12)",
                background: {
                    paper: "#fff",
                    default: "#fff"
                },
                action: {
                    active: "rgba(0, 0, 0, 0.54)",
                    hover: "rgba(0, 0, 0, 0.04)",
                    hoverOpacity: 0.04,
                    selected: "rgba(0, 0, 0, 0.08)",
                    selectedOpacity: 0.08,
                    disabled: "rgba(0, 0, 0, 0.26)",
                    disabledBackground: "rgba(0, 0, 0, 0.12)",
                    disabledOpacity: 0.38,
                    focus: "rgba(0, 0, 0, 0.12)",
                    focusOpacity: 0.12,
                    activatedOpacity: 0.12
                },
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
            }
        };
    default:
        break;
    }
};
const dark = createTheme(getPalette('dark'));
const light = createTheme(getPalette('light'));

const getTheme = (mode) => {
    switch(mode){
    case 'dark':
        return dark;
    default:
        return light;
    }
};
    

export default getTheme;
export {dark, light};