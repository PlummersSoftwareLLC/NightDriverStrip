const httpPrefix='';
const { useState, useEffect, useMemo, useRef, StrictMode } = window.React;

const { ThemeOptions, createTheme, ThemeProvider, Checkbox, AppBar, Toolbar, IconButton, Icon, MenuIcon, Typography } = window.MaterialUI;
const { Badge, withStyles, CssBaseline, Drawer, Divider, List, ListItem, ListItemIcon, ListItemText } = window.MaterialUI;
const { Box, Dialog, Slide, Button, TextField, FormControlLabel, useTheme, LinearProgress, Popover } = window.MaterialUI;
const { Card, CardHeader, CardContent, Collapse, CardActions, CardActionArea, Avatar, Link, Paper } = window.MaterialUI;
const { ExpandMore, ClickAwayListener } = window.MaterialUI;

const { AreaChart, BarChart, Area, Bar, ResponsiveContainer, LineChart, Line, CartesianGrid, XAxis, YAxis, Tooltip, Legend } = window.Recharts;

const getPalette = (mode) => {
  switch (mode) {
    case "dark":
      return {
        mode,
        common: {
            black: "#000",
            white: "#fff"
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
      }      
    case "light": 
      return {
        mode: mode,
        common: {
            black: "#000",
            white: "#fff"
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

    default:
      break;
  }
}

const getTheme = (mode) => createTheme({
  palette: getPalette(mode),
});

// const pannelText= {
//   fontFamily: ["Roboto", "Helvetica", "Arial", "sans-serif"].join(", ")
// };

// const commonTypography={
//   littleHeader: {
//       fontWeight: 500,
//       fontSize: "1.25rem",
//       lineHeight: 1.6,
//       letterSpacing: "0.0075em",
//   },
//   littleValue: {
//     lineHeight: 1.0,
//     fontSize: "3.75rem",
//     fontWeight: 300
//   }
// };
    
// const lightTheme = createTheme({
//     palette: {
//       mode: 'light',
//       type: 'light',
//       taskManager: {
//         strokeColor: '#90ff91',
//         MemoryColor: '#0002ff',
//         idleColor: 'black',
//         color1: '#58be59db',
//         color2: '#58be59a1',
//         color3: '#58be596b',
//         color4: '#58be5921',
//         bcolor1: '#189cdbff',
//         bcolor2: '#189cdba1',
//         bcolor3: '#189cdb66',
//         bcolor4: '#189cdb38',
//       }
//     },
//   typography: commonTypography,
// });

// const darkTheme = createTheme({
//   palette: {
//     mode: 'dark',
//     type: 'dark',
//     taskManager: {
//       strokeColor: '#90ff91',
//       MemoryColor: '#0002ff',
//       idleColor: 'black',
//       color1: '#58be59db',
//       color2: '#58be59a1',
//       color3: '#58be596b',
//       color4: '#58be5921',
//       bcolor1: '#189cdbff',
//       bcolor2: '#189cdba1',
//       bcolor3: '#189cdb66',
//       bcolor4: '#189cdb38',
//     },
//     text: {
//       primary: "#97ea44",
//       secondary: "aquamarine",
//       attribute: "aqua",
//       icon: "aquamarine"
//     },
//     primary: {
//       main: "#97ea44"
//     }
//   },
//   typography: commonTypography,
//   overrides: {
//     MuiAppBar: {
//       colorPrimary: {
//         backgroundColor: "black",
//         color: "textPrimary"
//       }
//     },
//     MuiIconButton: {
//       colorPrimary: {
//         color: "aquamarine"
//       },
//       colorSecondary: {
//         color: "aqua"
//       },
//       root:{
//         color: "lightgreen",
//       }
//     },
//     MuiCheckbox: {
//       colorPrimary: {
//         color: "textPrimary"
//       },
//       colorSecondary: {
//         &$checked: "aquamarine"
//       }
//     }
//   },
// });
const notificationsStyle = theme => ({
    root: {
    },
    popup: {
    },
    errorTarget: {
    },
    errors: {
        display: "flex",
        flexDirection: "column"
    },
    errorHeader: {
        display: "flex",
        flexDirection: "row",
        justifyContent: "space-between",
        borderBottom: "solid aquamarine 2px",
    }
});const drawerWidth = 240;

const mainAppStyle = theme => ({
    root: {
      display: 'flex'
    },
    appbar: {
      zIndex: theme.zIndex.drawer + 1,
      transition: theme.transitions.create(['width', 'margin'], {
        easing: theme.transitions.easing.sharp,
        duration: theme.transitions.duration.leavingScreen,
      }),
    },
    appbarOpened: {
      marginLeft: drawerWidth,
      width: `calc(100% - ${drawerWidth}px)`,
      transition: theme.transitions.create(['width', 'margin'], {
        easing: theme.transitions.easing.sharp,
        duration: theme.transitions.duration.enteringScreen,
      }),
    },
    categoryStats: {
        "display": "flex",
        "flex-direction": "row",
        "flex-wrap": "wrap",
        "align-content": "flex-start",
        "justify-content": "center",
        "align-items": "center"
    },
    toolbarTitle: {
      "flex-grow": 1
    },
    drawer: {
      whiteSpace: 'nowrap',
      "z-index": 0,
      width: drawerWidth,
      transition: theme.transitions.create('width', {
        easing: theme.transitions.easing.sharp,
        duration: theme.transitions.duration.enteringScreen,
      }),
    },
    drawerClosed: {
      overflowX: 'hidden',
      transition: theme.transitions.create('width', {
        easing: theme.transitions.easing.sharp,
        duration: theme.transitions.duration.leavingScreen,
      }),
      width: theme.spacing.unit * 7,
      [theme.breakpoints.up('sm')]: {
        width: theme.spacing.unit * 9,
      },
    },
    drawerHeader: {
      display: "flex",
      "flex-wrap": "nowrap",
      "min-height": "64px",
      "flex-direction": "row",
      "justify-content": "space-between"
    },
    displayMode: {
      display: "flex",
      "flex-wrap": "nowrap",
      "flex-direction": "row",
      "justify-content": "flex-start",
      "align-items": "center"
    },
    content: {
      padding: theme.spacing.unit * 10,
      transition: theme.transitions.create('padding-left', {
        easing: theme.transitions.easing.sharp,
        duration: theme.transitions.duration.leavingScreen,
      }),
      display: "flex",
      flexDirection: "row",
      flexWrap: "wrap",
      rowGap: "10px"
    },
    contentShrinked: {
      "padding-left": drawerWidth + 10,
      transition: theme.transitions.create('padding-left', {
        easing: theme.transitions.easing.sharp,
        duration: theme.transitions.duration.leavingScreen,
      })
    },
    optionSelected: {
      color: theme.palette.text.primary
    }
  });
  const configStyle = theme => ({
    configBar: {
        "padding-top": "65px"
    },
    configDisplay: {
        "display": "flex",
        "column-gap": "10px",
        "flex-direction": "column",
        "flex-wrap": "nowrap",
        "justify-content": "flex-start",
        "align-items": "center",
    },
    cblabel: {
        "margin-left": "initial"
    },
    hidden: {
        display: "none"
    }
});const designStyle = theme => ({
    root: {
        "display": "flex",
        "flex-direction": "column",
    },
    hidden: {
        display: "none"
    },
    effectsHeader: {
        display: "flex",
        flexDirection: "row",
        borderBottom: "solid 1px",
        columnGap: "5px",
    },
    effectsHeaderValue: {
        display: "flex",
        flexDirection: "row",
        columnGap: "3px",
        alignItems: "center",
    },
    effects: {
        display: "flex",
        flexDirection: "row",
        flexWrap: "wrap",
        padding: "10px",
        columnGap: "15px",
        rowGap: "15px",
        paddingTop: "20px",
    }
});const countdownStyle = theme => ({
    root: {
        display: "flex",
        flexDirection: "row",
        columnGap: "3px",
        alignItems: "center",
    },
    timeremaining: {
        width: "50px"
    }
});const effectStyle = theme => ({
    root: {
        "display": "flex",
        "flex-direction": "column",
        padding: "10px",
    },
    cardheader: {
    },
    hidden: {
        display: "none"
    },
    effect: {
        width: "180px",
        display: "flex",
        flexDirection: "column",
        flexWrap: "nowrap",
        justifyContent: "space-between",
    },
    effectPannel: {
        display: "flex",
        flexDirection: "row",
        alignItems: "center",
        justifyContent: "space-around",
    },
    effectName: {
        marginLeft: "10px",
        marginBottom: "5px"
    },
    unselected: {
        opacity: "30%"
    },
    selected: {
        backgroundColor: theme.palette.background.paper,
    }
});const statsStyle = theme => ({
    root: {
        "display": "flex",
        "flex-direction": "row",
        "flex-wrap": "wrap",
        "align-content": "flex-start",
        "justify-content": "flex-start",
        "align-items": "stretch",
        columnGap: "10px",
        rowGap: "10px"
    },
    summaryStats: {
        cursor: "pointer"
    },
    detiailedStats: {
        border: "solid 1px"
    },
    chartArea: {
        "display": "flex",
        "flex-direction": "row",
        "flex-wrap": "wrap",
        "align-content": "flex-start",
        "justify-content": "flex-start",
        "align-items": "flex-start"
    },
    chartHeader: {
        "display": "flex",
        "flex-direction": "column",
        "flex-wrap": "wrap",
        "align-content": "center",
        "justify-content": "flex-start",
        "align-items": "flex-start"
    },
    categoryStats:{
        "display": "flex",
        "flex-direction": "row",
        "flex-wrap": "wrap",
        "align-content": "flex-start",
        "justify-content": "space-between",
        "align-items": "flex-start",
        "column-gap": "5px",
    },
    category:{
    },
    hidden: {
        display: "none"
    },
    statCatergoryHeader: {
        "display": "flex",
        "flex-direction": "row",
        "flex-wrap": "wrap",
        "align-content": "flex-start",
        "justify-content": "space-between",
        "align-items": "center",
        "border-bottom": "solid 1px",
    }
});

const statStyle = theme => ({
    root: {
      display: 'flex',
      "flex-direction": "column"
    }
});
const staticStatStyle = theme => ({
    root: {
        "display": "flex",
        "flex-direction": "column",
        "flex-wrap": "wrap",
        "align-content": "flex-start",
        "justify-content": "space-between",
        "align-items": "flex-start",
        "margin-left": "5px",
        "margin-right": "5px"
    }
});
const areaChartStyle = theme => ({
    root: {
        "display": "flex",
        "flex-direction": "column",
        "flex-wrap": "wrap",
        "align-content": "flex-start",
        "justify-content": "flex-start",
        "align-items": "stretch"
    },
    header: {
        "display": "flex",
        "flex-direction": "column",
        "flex-wrap": "nowrap",
        "justify-content": "center",
        "align-items": "stretch"
    },
    headerLine: {
        "display": "flex",
        "flex-direction": "row",
        "flex-wrap": "nowrap",
        "justify-content": "space-between",
        "align-items": "center",
        "width": "100%"
    },
    headerField: {
        "display": "flex",
        "flex-direction": "row",
        "flex-wrap": "nowrap",
        "justify-content": "center",
        "align-items": "center",
        "width": "inherit"
    },
    stats: {
        "display": "flex",
        "flex-direction": "row",
        "flex-wrap": "nowrap",
        "justify-content": "center",
        "align-items": "center",
        "padding": "0px"
    },
    stat: {
        "display": "flex",
        "padding": "0px",
        "flex-wrap": "nowrap",
        "align-items": "center",
        "flex-direction": "row",
        "justify-content": "center"
    },
    tooltipContent: {
        "display": "flex",
        "flex-direction": "column",
        "flex-wrap": "nowrap",
        "align-content": "center",
        "justify-content": "center",
        "align-items": "stretch",
        "background-color": "black",
        "padding": "5px"
    },
    tooltipHeader: {
        "font-size": "medium",
        "display": "flex",
        "flex-direction": "column",
        "flex-wrap": "nowrap",
        "align-content": "center",
        "align-items": "center",
        "justify-content": "center",
        "border-bottom": "solid 1px"
    },
    threads: {
        "margin": "0px",
        "padding": "0px",
        "display": "flex",
        "flex-direction": "column",
        "flex-wrap": "nowrap",
        "align-content": "center",
        "justify-content": "center",
        "align-items": "stretch",
        "font-size": "small",
    },
    thread: {
        "display": "flex",
        "flex-direction": "row",
        "flex-wrap": "nowrap",
        "align-content": "center",
        "justify-content": "space-between",
        "align-items": "center",
        "column-gap": "5px",
    },
    threadValue: {
        "display": "flex",
        "flex-direction": "row",
        "flex-wrap": "nowrap",
        "align-content": "center",
        "justify-content": "center",
        "align-items": "center",
        "font-size": "smaller",
        "color": "aquamarine",
        "column-gap": "3px"
    },
    threadSummary: {
        "font-size": "x-small",
        "color": "aqua"
    }
});
const barChartStyle = theme => ({
    summary: {
        "display": "flex",
        "flex-direction": "column",
        "flex-wrap": "wrap",
        "align-content": "flex-start",
        "justify-content": "flex-start",
        "align-items": "center"
    },
    header: {
        "display": "flex",
        "flex-direction": "column",
        "flex-wrap": "nowrap",
        "justify-content": "center",
        "align-items": "stretch"
    },
    headerLine: {
        "display": "flex",
        "flex-direction": "row",
        "flex-wrap": "nowrap",
        "justify-content": "space-between",
        "align-items": "center",
        "width": "100%"
    },
    stats: {
        "display": "flex",
        "flex-direction": "row",
        "flex-wrap": "nowrap",
        "justify-content": "center",
        "align-items": "center",
        "padding": "0px"
    },
    stat: {
        "display": "flex",
        "padding": "0px",
        "flex-wrap": "nowrap",
        "align-items": "center",
        "flex-direction": "row",
        "justify-content": "center"
    },
    tooltipContent: {
        "display": "flex",
        "flex-direction": "column",
        "flex-wrap": "nowrap",
        "align-content": "center",
        "justify-content": "center",
        "align-items": "stretch",
        "background-color": "black",
        "padding": "5px"
    },
    tooltipHeader: {
        "font-size": "medium",
        "display": "flex",
        "flex-direction": "column",
        "flex-wrap": "nowrap",
        "align-content": "center",
        "align-items": "center",
        "justify-content": "center",
        "border-bottom": "solid 1px"
    },
    threads: {
        "margin": "0px",
        "padding": "0px",
        "display": "flex",
        "flex-direction": "column",
        "flex-wrap": "nowrap",
        "align-content": "center",
        "justify-content": "center",
        "align-items": "stretch",
        "font-size": "small",
    },
    thread: {
        "display": "flex",
        "flex-direction": "row",
        "flex-wrap": "nowrap",
        "align-content": "center",
        "justify-content": "space-between",
        "align-items": "center",
        "column-gap": "5px",
    },
    threadValue: {
        "display": "flex",
        "flex-direction": "row",
        "flex-wrap": "nowrap",
        "align-content": "center",
        "justify-content": "center",
        "align-items": "center",
        "font-size": "smaller",
        "color": "aquamarine",
        "column-gap": "3px"
    },
    threadSummary: {
        "font-size": "x-small",
        "color": "aqua"
    }
});
const NotificationPanel = withStyles(notificationsStyle)(props => {
    const { classes, notifications, clearNotifications } = props;
    const [numErrors, setNumErrors] = React.useState(undefined);
    const [errorTargets, setErrorTargets] = React.useState({});
    const [open, setOpen] = React.useState(false);
    const inputRef = React.createRef();
    const theme = useTheme();

    useEffect(()=>{
        setNumErrors(notifications.reduce((ret,notif) => ret+notif.notifications.length, 0));
        setErrorTargets(notifications.reduce((ret,notif) => 
            {return {...ret,[notif.target]:ret[notif.target] || false}}, {}));
    },[notifications]);

    return (
        <Box className={classes.root}>
            <IconButton
                    id="notifications"
                    ref={inputRef}
                    onClick={() => setOpen(wasOpen=>!wasOpen)}>
                <Badge 
                    aria-label="Alerts" 
                    badgeContent={numErrors} 
                    color="secondary">
                    <Icon>notifications</Icon>
                </Badge>
            </IconButton>
            <Popover
                open={open}
                target="notifications"
                onClose={()=>{setOpen(false)}}
                anchorOrigin={{
                    vertical: 'top',
                    horizontal: 'right',
                }}>
                <Card className={classes.popup} elevation={9}>
                    <CardHeader
                        avatar={
                        <Avatar sx={{ bgcolor: theme.palette.error.dark }} aria-label="error">
                            !
                        </Avatar>
                        }
                        action={
                        <IconButton onClick={()=>setOpen(false)} aria-label="settings">
                            <Icon>close</Icon>
                        </IconButton>
                        }
                        title={`${numErrors} Errors`}
                    />
                    <CardContent>
                        {Object.entries(errorTargets)
                               .sort((a,b)=>a[0].localeCompare(b[0]))
                               .map(target => 
                            <CardContent key={target[0]} className={classes.errors}>
                                {Object.entries(notifications)
                                       .filter(notif => notif[1].target === target[0])
                                       .map(error =>
                                <Box key={error[0]}>
                                    <Box className={classes.errorHeader} key="header">
                                        <Typography>{target[0]}</Typography>
                                        <Typography color="textSecondary">{error[1].type}</Typography>
                                        <Typography>{error[1].level}</Typography>
                                    </Box>
                                    <Box className={classes.errors} key="errors">
                                        {Object.entries(error[1].notifications.reduce((ret,error) => {return {...ret,[error.notification]:(ret[error.notification]||0)+1}},{}))
                                                .map(entry => <Typography key={entry[1]} variant="tiny">{`${entry[1]}X ${entry[0]}`}</Typography>)
                                        }
                                    </Box>
                                </Box>
                                       )}
                            </CardContent>
                        )}
                    </CardContent>
                    <CardActions disableSpacing>
                        <IconButton onClick={()=>clearNotifications()} aria-label="Clear Errors">
                            <Icon>delete</Icon>
                        </IconButton>
                    </CardActions>
                </Card>
            </Popover>
        </Box>);
});const MainApp = () => {
    const [mode, setMode] = useState('dark');
    const theme = React.useMemo(
        () => getTheme(mode),[mode]);
    return <ThemeProvider theme={theme}><CssBaseline /><AppPannel mode={mode} setMode={setMode} /></ThemeProvider>
};

const AppPannel = withStyles(mainAppStyle)(props => {
    const { classes, mode, setMode } = props;
    const [drawerOpened, setDrawerOpened] = useState(false);
    const [stats, setStats] = useState(false);
    const [designer, setDesigner] = useState(false);
    const [statsRefreshRate, setStatsRefreshRate ] = useState(3);
    const [maxSamples, setMaxSamples ] = useState(50);
    const [animateChart, setAnimateChart ] = useState(false);
    const [notifications, setNotifications] = useState([]);

    const siteConfig = {
        statsRefreshRate: {
            name: "Refresh rate",
            value: statsRefreshRate,
            setter: setStatsRefreshRate,
            type: "int"
        },
        statsAnimateChange: {
            name: "Animate chart",
            value: animateChart,
            setter: setAnimateChart,
            type: "boolean"
        },
        maxSamples: {
            name: "Chart points",
            value: maxSamples,
            setter: setMaxSamples,
            type: "int"
        }
    };

    const addNotification = (level,type,target,notification) => {
        setNotifications(prevNotifs => {
            const group = prevNotifs.find(notif=>(notif.level === level) && (notif.type == type) && (notif.target === target)) || {level,type,target,notifications:[]};
            group.notifications.push({date:new Date(),notification});
            return [...prevNotifs.filter(notif => notif !== group), group];
        });
    };

    return <Box className={classes.root}>
            <AppBar className={[classes.appbar,drawerOpened && classes.appbarOpened].join(" ")}>
                <Toolbar>
                    <IconButton 
                        aria-label="Open drawer" 
                        onClick={()=>setDrawerOpened(!drawerOpened)} 
                        className={drawerOpened && classes.drawerClosed}>
                        <Icon>{drawerOpened ? "chevron" : "menu"}</Icon>
                    </IconButton>
                    <Typography
                        className={classes.toolbarTitle}
                        component="h1"
                        variant="h6">
                        Night Driver Strip
                    </Typography>
                    {(notifications.length > 0) && <NotificationPanel notifications={notifications} clearNotifications={()=>setNotifications([])}/>}
                </Toolbar>
            </AppBar>
            <Drawer variant="permanent" 
                    classes={{paper: [classes.drawer, !drawerOpened && classes.drawerClosed].join(" ")}}>
                <Box className={classes.drawerHeader}>
                    <Box className={classes.displayMode}>
                        <IconButton onClick={()=>setMode(mode === "dark" ? "light" : "dark")} ><Icon>{mode === "dark" ? "dark_mode" : "light_mode"}</Icon></IconButton>
                        <ListItemText primary={(mode === "dark" ? "Dark" : "Light") + " mode"}/>
                    </Box>
                    <IconButton onClick={()=>setDrawerOpened(!drawerOpened)}>
                        <Icon>chevron_left</Icon>
                    </IconButton>
                </Box>
                <Divider/>
                <List>{
                    [{caption:"Home", flag: designer, setter: setDesigner, icon: "home"},
                     {caption:"Statistics", flag: stats, setter: setStats, icon: "area_chart"},
                     {caption:"", flag: drawerOpened, icon: "settings", setter: setDrawerOpened}].map(item => 
                    <ListItem key={item.icon}>
                        <ListItemIcon><IconButton onClick={() => item.setter && item.setter(prevValue => !prevValue)}>
                            <Icon color="action" className={item.flag && classes.optionSelected}>{item.icon}</Icon>
                        </IconButton></ListItemIcon>
                        <ListItemText primary={item.caption}/>
                        {drawerOpened && (item.icon === "settings") && <ConfigPanel siteConfig={siteConfig} />}
                    </ListItem>)
                }</List>
            </Drawer>
            <Box className={[classes.content, drawerOpened && classes.contentShrinked].join(" ")}>
                <StatsPanel siteConfig={siteConfig} open={stats} addNotification={addNotification}/> 
                <DesignerPanel siteConfig={siteConfig} open={designer} addNotification={addNotification}/>
            </Box>
        </Box>;
});
const ConfigItem = withStyles(configStyle)(props => {
    const { name, value, configItemUpdated, datatype, classes } = props;
    const [ editing, setEditing] = useState(false);
    const [ configValue, setConfigValue] = useState(value);
    const getConfigValue = (value, type) => {
        switch (type) {
            case "int":
                return parseInt(value)
            case "float":
                return parseFloat(value)
            default:
                return value;
        }
    }

    if (datatype === "boolean") {
        return <ListItem button onClick={_evt=>!editing && setEditing(!editing)}>
            <FormControlLabel
                className={classes.cblabel}
                label={name} 
                labelPlacement="top"
                control={<Checkbox 
                    defaultChecked={value}
                    onChange={event => {
                        setConfigValue(event.target.checked);
                        configItemUpdated(event.target.checked);
                    }} />} />
        </ListItem>;
    }

    return <ClickAwayListener onClickAway={()=>{configItemUpdated(configValue);setEditing(false);}}><ListItem button onClick={_evt=>!editing && setEditing(!editing)}>
                {!editing && <ListItemText className={ classes.configDisplay }
                    primary={name}
                    secondary={configValue}/>}
                {editing && <TextField label={name} 
                                       variant="outlined"
                                       type={["int","float"].includes(datatype) ? "number" : "text"}
                                       pattern={datatype === "int" ? "^[0-9]+$" : (datatype === "float" ? "^[0-9]+[.0-9]*$" : ".*")}
                                       defaultValue={value}
                                       onChange={event => setConfigValue(getConfigValue(event.target.value,datatype)) } />}
            </ListItem></ClickAwayListener>;
});
const ConfigPanel = withStyles(configStyle)(props => {
  const { classes, siteConfig } = props;
  return (
            <List>
                {Object.entries(siteConfig).map(entry => <ConfigItem 
                                            name={entry[1].name}
                                            key={entry[1].name}
                                            datatype={entry[1].type}
                                            value={entry[1].value}
                                            configItemUpdated={value => entry[1].setter(value)} 
                                            />)}
            </List>
  );
});const DesignerPanel = withStyles(designStyle)(props => {
    const { classes, open, addNotification } = props;
    const [ effects, setEffects ] = useState(undefined);
    const [ abortControler, setAbortControler ] = useState(undefined);
    const [ nextRefreshDate, setNextRefreshDate] = useState(undefined);
    const [ editing, setEditing ] = useState(false);
    const [ requestRunning, setRequestRunning ] = useState(false);
    const [ pendingInterval, setPendingInterval ] = useState(effects && effects.effectInterval);

    useEffect(() => {
        if (abortControler) {
            abortControler.abort();
        }

        if (open) {
            const aborter = new AbortController();
            setAbortControler(aborter);

            const timer = setTimeout(()=>{
                setNextRefreshDate(Date.now());
            },3000);
    
            chipRequest(`${httpPrefix !== undefined ? httpPrefix : ""}/getEffectList`,{signal:aborter.signal})
                .then(resp => resp.json())
                .then(setEffects)
                .then(()=>clearTimeout(timer))
                .catch(err => addNotification("Error","Service","Get Effect List",err));
    
            return () => {
                abortControler && abortControler.abort();
                clearTimeout(timer);
            }
        }
    },[open,nextRefreshDate]);

    const requestRefresh = () => setTimeout(()=>setNextRefreshDate(Date.now()),50);

    const chipRequest = (url,options,operation) => 
        new Promise((resolve,reject) => 
            fetch(url,options)
                .then(resolve)
                .catch(err => {addNotification("Error",operation,err);reject(err)}));

    const navigateTo = (idx)=>{
        return new Promise((resolve,reject)=>{
            setRequestRunning(true);
            chipRequest(`${httpPrefix !== undefined ? httpPrefix : ""}/setCurrentEffectIndex?`,{method:"POST", body: new URLSearchParams({currentEffectIndex:idx})}, "navigateTo")
                .then(resolve)
                .then(requestRefresh)
                .catch(reject)
                .finally(()=>setRequestRunning(false));    
        });
    }

    const effectEnable = (idx,enable)=>{
        return new Promise((resolve,reject)=>{
            setRequestRunning(true);
            chipRequest(`${httpPrefix !== undefined ? httpPrefix : ""}/${enable?"enable":"disable"}Effect`,{method:"POST", body:new URLSearchParams({effectIndex:idx})},"effectEnable")
                .then(resolve)
                .then(requestRefresh)
                .catch(reject)
                .finally(()=>setRequestRunning(false));    
        });
    }

    const navigate = (up)=>{
        return new Promise((resolve,reject)=>{
            setRequestRunning(true);
            chipRequest(`${httpPrefix !== undefined ? httpPrefix : ""}/${up ? "nextEffect" : "previousEffect"}`,{method:"POST"},"nvigate")
                .then(resolve)
                .then(requestRefresh)
                .catch(reject)
                .finally(()=>setRequestRunning(false));    
        });
    }

    const updateEventInterval = (interval)=>{
        return new Promise((resolve,reject)=>{
            setRequestRunning(true);
            chipRequest(`${httpPrefix !== undefined ? httpPrefix : ""}/settings`,
            {
                method:"POST",
                body: new URLSearchParams({effectInterval:interval})
            },"updateEventInterval").then(resolve)
              .then(requestRefresh)
              .catch(reject)
              .finally(()=>setRequestRunning(false));    
        });
    }

    const displayHeader = ()=>{
        return <Box className={classes.effectsHeaderValue}>
            <Typography variant="little" color="textPrimary">Interval</Typography>:
            <Link href="#" variant="little" color="textSecondary" onClick={() => setEditing(true)}>{effects.effectInterval}</Link>
        </Box>;
    }

    const editingHeader = ()=>{
        return <ClickAwayListener onClickAway={()=>{updateEventInterval(pendingInterval);setEditing(false);}}>
            <Box className={classes.effectsHeaderValue}>
            <TextField label="Interval ms"
                variant="outlined"
                type="number"
                defaultValue={effects.effectInterval}
                onChange={event => setPendingInterval(event.target.value)} />
        </Box></ClickAwayListener>;
    }

    if (!effects && open){
        return <Box>Loading....</Box>;
    }

    return effects && <Box className={`${classes.root} ${!open && classes.hidden}`}>
        <Box className={classes.effectsHeader}>
            {editing ? 
            editingHeader():
            displayHeader()}
            <Countdown 
                label="Time Remaining"
                requestRefresh={requestRefresh}
                millisecondsRemaining={effects.millisecondsRemaining}/>
            {(effects.Effects.length > 1) && <Box>
                <IconButton disabled={requestRunning} onClick={()=>navigate(false)}><Icon>skip_previous</Icon></IconButton>
                <IconButton disabled={requestRunning} onClick={()=>navigate(true)}><Icon>skip_next</Icon></IconButton>
                <IconButton disabled={requestRunning} onClick={()=>setNextRefreshDate(Date.now())}><Icon>refresh</Icon></IconButton>
            </Box>}
        </Box>
        <Box className={classes.effects}>
            {effects.Effects.map((effect,idx) => <Effect 
                                                    key={`effect-${idx}`}
                                                    effect={effect} 
                                                    effectIndex={idx} 
                                                    navigateTo={navigateTo}
                                                    requestRunning={requestRunning}
                                                    effectEnable={effectEnable}
                                                    effectInterval={effects.effectInterval}
                                                    selected={idx === effects.currentEffect}
                                                    millisecondsRemaining={effects.millisecondsRemaining}/>)}
        </Box>
    </Box>
});const Countdown = withStyles(countdownStyle)(props => {
    const { classes,  label, millisecondsRemaining, requestRefresh } = props;
    const [ timeRemaining, setTimeRemaining ] = useState(false);

    useEffect(() => {
        if (millisecondsRemaining) {
            const timeReference = Date.now()+millisecondsRemaining;
            setTimeRemaining(timeReference-Date.now());
            var requestSent = false;
            const interval = setInterval(()=>{
                const remaining = timeReference-Date.now();
                if (remaining >= 0) {
                    setTimeRemaining(remaining);
                }
                if ((remaining <= 100) && !requestSent) {
                    requestSent=true;
                    requestRefresh();
                }
            },50);
            return ()=>clearInterval(interval);
        }
    },[millisecondsRemaining]);

    return (            
    <Box className={classes.root}>
        <Typography variant="little" color="textPrimary">{label}</Typography>:
        <Typography color="textSecondary" className={classes.timeremaining} width="100px" variant="little">{timeRemaining}</Typography>
    </Box>)

});const Effect = withStyles(effectStyle)(props => {
    const { classes, effect, effectInterval, effectIndex, millisecondsRemaining, selected, effectEnable, navigateTo, requestRunning } = props;
    const [ progress, setProgress ] = useState(undefined);
    const [expanded, setExpanded] = React.useState(false);

    useEffect(() => {
        if (millisecondsRemaining && selected) {
            const timeReference = Date.now()+millisecondsRemaining;
            var timeRemaining = timeReference-Date.now();
            const interval = setInterval(()=>{
                const remaining = timeReference-Date.now();
                if (remaining >= 0) {
                    timeRemaining = remaining;
                    setProgress((timeRemaining/effectInterval)*100.0);
                }
            },300);
            return ()=>clearInterval(interval);
        }
        if (!selected) {
            setProgress(0);
        }
    },[millisecondsRemaining,selected]);

    return <Card variant="outlined" className={classes.effect}>
                <CardHeader
                    avatar={
                        <Avatar aria-label={effect.name}>
                            {effect.name[0]}
                        </Avatar>
                      }
                    title={effect.name}
                    subheader={effect.enabled?(selected?"Active":"Waiting") : "Disabled"}
                    className={classes.cardheader}
                /> 
                <CardContent>
                    {selected && <LinearProgress disabled={requestRunning} variant="determinate" sx={{transition: 'none'}} value={progress}/>}
                    {!selected && <Button disabled={requestRunning} onClick={()=>effectEnable(effectIndex,!effect.enabled)} variant="outlined" startIcon={<Icon >{effect.enabled?"stop":"circle"}</Icon>}>{effect.enabled?"Disable":"Enable"}</Button>}
                    {!selected && effect.enabled && <Button disabled={requestRunning} onClick={()=>navigateTo(effectIndex)} variant="outlined" startIcon={<Icon >start</Icon>}>Trigger</Button>}
                </CardContent>
                <CardActions disableSpacing>
                    <IconButton
                        onClick={()=>setExpanded(!expanded)}
                        aria-label="show more">
                        <Icon>settings</Icon>
                    </IconButton>
                </CardActions>
                <Collapse in={expanded} timeout="auto" unmountOnExit>
                    <CardContent>
                        <TextField label="Option"/>
                    </CardContent>
                </Collapse>
            </Card>
});const StaticStatsPanel = withStyles(staticStatStyle)(props => {
    const { classes, stat, name, detail } = props;

    return <Box className={classes.root}>
        <Typography variant={detail ? "h5" : "h6"}>{name}</Typography>
        {detail ? <List>
            {Object.entries(stat.stat)
                   .map(entry=>
                <ListItem key={entry[0]}>
                    <Typography variant="little" color="textPrimary">{entry[0]}</Typography>:
                    <Typography variant="little" color="textSecondary">{entry[1]}</Typography>
                </ListItem>)}
        </List>:
        <List>
        {Object.entries(stat.stat)
               .filter(entry => stat.headerFields.includes(entry[0]))
               .map(entry=><Typography key={entry[0]} variant="little" color="textSecondary" >{entry[1]}</Typography>)}
    </List>}
    </Box>
});const AreaStat = withStyles(areaChartStyle)(props => {
    const { classes, name, rawvalue, ignored, statsAnimateChange, maxSamples, headerFields , idleField, category, detail } = props;
    const getChartValues = (value) => Object.entries(value)
                        .filter(entry=>!ignored.includes(entry[0]))
                        .reduce((ret,entry)=>{ret[entry[0]] = entry[1]; return ret},{});
    const [lastStates, setLastStates] = useState([Object.entries(getChartValues(rawvalue))
        .filter(entry=>!ignored.includes(entry[0]))
        .reduce((ret,stat)=>{ret[stat[0]]=stat[1]; return ret},{ts: new Date().getTime()})] );
    const getValue = (value) => value !== undefined && !Number.isInteger(value) ? (isNaN(value) ? value : value.toFixed(2)) : value;
    const theme = useTheme();

    useMemo(()=>{
        setLastStates(lastStates === undefined ? [Object.entries(getChartValues(rawvalue))] : [...lastStates,Object.entries(getChartValues(rawvalue))
            .filter(entry=>!ignored.includes(entry[0]))
            .reduce((ret,stat)=>{ret[stat[0]]=stat[1]; return ret},{ts: new Date().getTime()})]
            .filter((_val,idx,arr) => arr.length >= maxSamples ? idx > arr.length - maxSamples : true));
    },[rawvalue]);
    
    const getFillColor = ({step, isIdle}) => {
        if (isIdle) {
            return theme.palette.taskManager.idleColor;
        }
        return (theme.palette.taskManager[`${category === "Memory" ? "b" : ""}color${step+1}`]);
    }

    const getStatTooltip = (data, classes) => {
        return (
        <div className={classes.tooltipContent}>
            <div className={classes.tooltipHeader}>{data.labelFormatter(data.label)}</div>
            <ul className={classes.threads}>
                {data.payload
                    .sort((a,b) => sortStats(b,a))
                    .map(stat => 
                    <div key={stat.name} className={classes.thread}>
                        <div className={classes.threadName} style={{color:stat.color}}>{stat.name}</div>
                        <div className={classes.threadValue}>{getValue(stat.value)}
                            <div className={classes.threadSummary}>
                                ({(stat.value/data.payload.reduce((ret,stat) => ret + stat.value,0)*100).toFixed(2)}%)
                            </div>
                        </div>
                    </div>)
                }
            </ul>
        </div>)
    }

    return <Box className={classes.root}>
        {detail && <Box className={classes.header}>
            <Typography className={classes.headerLine} color="textPrimary" variant="subtitle1">{name} {headerFields && Object.values(headerFields).map(headerField=>
                <Typography key={headerField} className={classes.headerField} color="textPrimary" variant="subtitle2">{headerField}: 
                    <Typography color="textSecondary" variant="subtitle2">{rawvalue[headerField]}</Typography>
                </Typography>)}
            </Typography>
            <List className={classes.stats}>
                {Object.entries(rawvalue)
                        .filter(entry=>!ignored.includes(entry[0]))
                        .map(entry=>
                    <ListItem className={classes.stats} key={entry[0]}>
                        <Typography color="textPrimary" variant="little">{entry[0]}</Typography>:
                        <Typography color="textSecondary" variant="little" >{getValue(entry[1])}</Typography>
                    </ListItem>)}
            </List>
        </Box>}
        <AreaChart 
            data={lastStates}
            height={detail ? 300 : 80}
            width={detail ? 500 : 200}
            stackOffset="expand">
            <defs>
                {Object.entries(getChartValues(rawvalue))
                       .filter(entry => entry[1] !== undefined)
                       .map((entry,idx,arr) => <linearGradient key={`color${entry[0]}`} id={`color${entry[0]}`} x1="0" y1="0" x2="0" y2="1">
                                                <stop offset="5%" stopColor={getFillColor({numOfSteps: arr.length, step: idx, isIdle: entry[0] === idleField})} stopOpacity={0.8}/>
                                                <stop offset="95%" stopColor={getFillColor({numOfSteps: arr.length, step: idx, isIdle: entry[0] === idleField})} stopOpacity={0}/>
                                              </linearGradient>)}
            </defs>
            <XAxis dataKey="ts"
                   name='Time'
                   hide={!detail}
                   tickFormatter={unixTime => new Date(unixTime).toLocaleTimeString()}></XAxis>
            <YAxis hide={true}></YAxis>
            <CartesianGrid strokeDasharray="3 3"/>
            {<Tooltip content={data => getStatTooltip(data, classes)}
                     labelFormatter={t => new Date(t).toLocaleString()}></Tooltip>}
            {Object.entries(getChartValues(rawvalue))
                    .filter(entry => entry[1] !== undefined)
                    .sort((a,b) => sortStats({name:a[0],chartValue:a[1]},{name:b[0],chartValue:b[1]}))
                    .map((entry) => 
                            <Area
                                key={entry[0]}
                                isAnimationActive={statsAnimateChange}
                                type="monotone"
                                fillOpacity={1} 
                                fill={`url(#color${entry[0]})`}
                                stroke={category === "Memory" ? theme.palette.taskManager.memoryColor : theme.palette.taskManager.strokeColor}
                                dataKey={entry[0]}
                                stackId="1"/>)}
        </AreaChart>
    </Box>

    function sortStats(a, b) {
        return a.name === idleField && b.name !== idleField ? 1 : (a.name !== idleField && b.name === idleField ? -1 : a.value-b.value);
    }
});
    const StatsPanel = withStyles(statsStyle)(props => {
    const { classes, siteConfig, open, addNotification } = props;
    const { statsRefreshRate, statsAnimateChange, maxSamples } = siteConfig;
    const [ statistics, setStatistics] = useState(undefined);
    const [ timer, setTimer ] = useState(undefined);
    const [ lastRefreshDate, setLastRefreshDate] = useState(undefined);
    const [ abortControler, setAbortControler ] = useState(undefined);
    const [ openedCategories, setOpenedCategories ] = useState({
        Package:false,
        CPU: false,
        Memory: false,
        NightDriver: false
    });

    const getStats = (aborter) => fetch(`${httpPrefix !== undefined ? httpPrefix : ""}/getStatistics`,{signal:aborter.signal})
                            .then(resp => resp.json())
                            .then(stats => {
                                setAbortControler(undefined); 
                                return {
                                    CPU:{
                                        CPU: {
                                            stat:{
                                                CORE0: stats.CPU_USED_CORE0,
                                                CORE1: stats.CPU_USED_CORE1,
                                                IDLE: ((200.0 - stats.CPU_USED_CORE0 - stats.CPU_USED_CORE1)/200)*100.0,
                                                USED: stats.CPU_USED
                                            },
                                            idleField: "IDLE",
                                            ignored: ["USED"],
                                            headerFields: ["USED"]
                                        }
                                    },
                                    Memory: {
                                        HEAP:{
                                            stat:{
                                                USED:stats.HEAP_SIZE-stats.HEAP_FREE,
                                                FREE:stats.HEAP_FREE,
                                                MIN:stats.HEAP_MIN,
                                                SIZE: stats.HEAP_SIZE
                                            },
                                            idleField: "FREE",
                                            headerFields: ["SIZE","MIN"],
                                            ignored:["SIZE","MIN"]
                                        },
                                        DMA: {
                                            stat:{
                                                USED: stats.DMA_SIZE - stats.DMA_FREE,
                                                FREE: stats.DMA_FREE,
                                                MIN: stats.DMA_MIN,
                                                SIZE: stats.DMA_SIZE
                                            },
                                            idleField: "FREE",
                                            headerFields: ["SIZE","MIN"],
                                            ignored:["SIZE","MIN"]
                                        },
                                        PSRAM: {
                                            stat:{
                                                USED: stats.PSRAM_SIZE - stats.PSRAM_FREE,
                                                FREE: stats.PSRAM_FREE,
                                                MIN: stats.PSRAM_MIN,
                                                SIZE: stats.PSRAM_SIZE
                                            },
                                            idleField: "FREE",
                                            headerFields: ["SIZE","MIN"],
                                            ignored:["SIZE","MIN"]
                                        },
                                    },
                                    NightDriver: {
                                        FPS:{
                                            stat:{
                                                LED:stats.LED_FPS,
                                                SERIAL:stats.SERIAL_FPS,
                                                AUDIO:stats.AUDIO_FPS
                                            }
                                        },
                                    },
                                    Package: {
                                        CHIP: {
                                            stat:{
                                                MODEL: stats.CHIP_MODEL,
                                                CORES: stats.CHIP_CORES,
                                                SPEED: stats.CHIP_SPEED,
                                                PROG_SIZE: stats.PROG_SIZE
                                            },
                                            static: true,
                                            headerFields: ["MODEL"]
                                        },
                                        CODE: {
                                            stat:{
                                                SIZE: stats.CODE_SIZE,
                                                FREE: stats.CODE_FREE,
                                                FLASH_SIZE: stats.FLASH_SIZE
                                            },
                                            static: true,
                                            headerFields: ["SIZE"]
                                        },
                                    },
                                };
                            });

    useEffect(() => {
        if (abortControler) {
            abortControler.abort();
        }

        if (open) {
            const aborter = new AbortController();
            setAbortControler(aborter);
    
            getStats(aborter)
                .then(setStatistics)
                .catch(err => addNotification("Error","Service","Get Statistics",err));
    
            if (timer) {
                clearTimeout(timer);
                setTimer(undefined);
            }
    
            if (statsRefreshRate.value && open) {
                setTimer(setTimeout(() => setLastRefreshDate(Date.now()),statsRefreshRate.value*1000));
            }
    
            return () => {
                timer && clearTimeout(timer);
                abortControler && abortControler.abort();
            }
        }
    },[statsRefreshRate.value, lastRefreshDate, open]);

    if (!statistics && open) {
        return <Box>Loading...</Box>
    }

    return statistics && 
    <Box className={`${classes.root} ${!open && classes.hidden}`}>
        {Object.entries(statistics).map(category => 
        <Box key={category[0]} className={`${classes.category} ${!openedCategories[category[0]] && classes.detailedStats}`}>
            {openedCategories[category[0]] ? 
            <Box className={classes.statCatergoryHeader} key="header">
                <Typography variant="h5">{category[0]}</Typography>
                <IconButton onClick={()=>setOpenedCategories(prev => {return {...prev,[category[0]]:!openedCategories[category[0]]}})}><Icon>minimize</Icon></IconButton>
            </Box>:
            <Box>
                <Typography color="textPrimary">{category[0]}</Typography>
            </Box>}
            <Box className={classes.categoryStats}>
            {Object.entries(category[1])
               .filter(entry=> entry[1].static) 
               .map(entry=>
                <Box 
                  key={`entry-${entry[0]}`}
                  className={!openedCategories[category[0]] && classes.summaryStats }  
                  onClick={()=>!openedCategories[category[0]] && setOpenedCategories(prev => {return {...prev,[category[0]]:!openedCategories[category[0]]}})}>
                    <StaticStatsPanel
                        key={`static-${entry[0]}`}
                        detail={openedCategories[category[0]]}
                        name={entry[0]}
                        stat={entry[1]}/>                
                </Box>)}
                <Box className={classes.categoryStats} key="charts">
                    {Object.entries(category[1])
                        .filter(entry=> !entry[1].static) 
                        .map((entry)=>  
                            <Box key={`chart-${entry[0]}`}
                                 sx={{cursor:"pointer"}}
                                 onClick={()=>!openedCategories[category[0]] && setOpenedCategories(prev => {return {...prev,[category[0]]:!openedCategories[category[0]]}})} 
                                 className={`${classes.chartArea} ${!openedCategories[category[0]] && classes.summaryStats}`}>
                                    {category[1][entry[0]].idleField && <BarStat
                                        key={`Bar-${entry[0]}`}
                                        name={entry[0]}
                                        className={entry[0]}
                                        category={category[0]}
                                        detail={openedCategories[category[0]]}
                                        rawvalue={entry[1].stat}
                                        idleField={ category[1][entry[0]].idleField }
                                        statsAnimateChange={ statsAnimateChange.value }
                                        headerFields={ category[1][entry[0]].headerFields }
                                        ignored={ category[1][entry[0]].ignored || [] } />}
                                    <AreaStat
                                        key={`Area-${entry[0]}`}
                                        name={entry[0]}
                                        category={category[0]}
                                        detail={openedCategories[category[0]]}
                                        statsAnimateChange={ statsAnimateChange.value }
                                        rawvalue={entry[1].stat}
                                        maxSamples={ maxSamples.value }
                                        idleField={ category[1][entry[0]].idleField }
                                        headerFields={ category[1][entry[0]].headerFields }
                                        ignored={ category[1][entry[0]].ignored || [] } />
                                </Box>)}
                </Box>
            </Box>
        </Box>)}
    </Box>
});

const BarStat = withStyles(barChartStyle)(props => {
    const { classes, name, rawvalue, ignored, statsAnimateChange , idleField, category, detail } = props;
    const theme = useTheme();

    const getFillColor = ({step, isIdle}) => {
        if (isIdle) {
            return theme.palette.taskManager.idleColor;
        }
        return (theme.palette.taskManager[`${category === "Memory" ? "b" : ""}color${step+1}`]);
    }

    return (
    <Box className={classes.summary}>
        <BarChart
            height={detail ? 300 : 70}
            width={detail ? 150 : 75}
            data={[Object.entries(rawvalue)
                .filter(entry=>!["name",...ignored].includes(entry[0]))
                .reduce((ret,entry)=>{ret[entry[0]] = entry[1]; return ret},{name:name})]}>
            <XAxis hide={true} dataKey="name" />
            <YAxis hide={true} />
            {Object.keys(rawvalue)
                    .filter(field => !ignored.includes(field))
                    .sort(sortStats)
                    .map((field,idx) => <Bar dataKey={field} 
                                                key={field}
                                                stackId="a" 
                                                fill={getFillColor({step: idx, isIdle: field === idleField})} 
                                                isAnimationActive={statsAnimateChange}
                                                type="monotone"
                                                fillOpacity={1}/>)
            }
        </BarChart>
        <Typography variant="summary">{(Object.entries(rawvalue)
                                             .filter(entry => ![idleField,...ignored].includes(entry[0]))
                                             .reduce((ret,stat)=>ret+stat[1],0.0)/
                                       Object.entries(rawvalue)
                                             .filter(entry => !ignored.includes(entry[0]))
                                             .reduce((ret,stat)=>ret+stat[1],0.0)*100).toFixed(0)}%</Typography>
    </Box>)

    function sortStats(a, b) {
        return a === idleField && b !== idleField ? 1 : (a !== idleField && b === idleField ? -1 : a.localeCompare(b));
    }
});
    ReactDOM.createRoot(document.getElementById("root"))
        .render(<StrictMode><MainApp/></StrictMode>);
