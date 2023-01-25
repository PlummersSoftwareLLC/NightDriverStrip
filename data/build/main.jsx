const { useState } = window.React;

const { createTheme, ThemeProvider, Checkbox, AppBar, Toolbar, IconButton, Icon, MenuIcon, Typography } = window.MaterialUI;
const { Badge, withStyles, CssBaseline, Drawer, Divider, List, ListItem, ListItemIcon, ListItemText } = window.MaterialUI;
const { Box } = window.MaterialUI;
const lightTheme = createTheme({
    palette: {
      mode: 'light',
      type: 'light'
    },
});

const darkTheme = createTheme({
  palette: {
    mode: 'dark',
    type: 'dark'
  }
});
const statsStyle = theme => ({
    root: {
        "display": "flex",
        "flex-direction": "row",
        "flex-wrap": "wrap",
        "align-content": "flex-start",
        "justify-content": "flex-start",
        "align-items": "flex-start"
    }
});

const statStyle = theme => ({
    root: {
      display: 'flex'
    }
});
const drawerWidth = 240;

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
      flexGrow: 1,
      padding: theme.spacing.unit * 10,
      overflow: 'auto',
      transition: theme.transitions.create('padding-left', {
        easing: theme.transitions.easing.sharp,
        duration: theme.transitions.duration.leavingScreen,
      })
    },
    contentShrinked: {
      "padding-left": drawerWidth + 10,
      transition: theme.transitions.create('padding-left', {
        easing: theme.transitions.easing.sharp,
        duration: theme.transitions.duration.leavingScreen,
      })
    }
  });const Stat = withStyles(statStyle)(props => {
const { classes, name, value } = props;
return <Box className={classes.root}>
    <Typography variant="h7">{name}</Typography>
    <List>
        {Object.entries(value).map(entry=><ListItem key={entry[0]}>
            <Typography variant="subtitle1">{entry[0]}</Typography>:
            <Typography variant="subtitle2">{entry[1]}</Typography>
        </ListItem>)}
    </List>
</Box>});


const StatsPanel = withStyles(statsStyle)(props => {
const { classes } = props;
const [statisics, setStatistics] = React.useState({
    FPS:{
        LED_FPS:25,
        SERIAL_FPS: 15,
        AUDIO_FPS: 23
    },
    HEAP:{
        HEAP_SIZE:9000,
        HEAP_FREE:4096,
        HEAP_MIN:2048
    },
    DMA: {
        DMA_SIZE: 1034,
        DMA_FREE: 1034,
        DMA_MIN: 1034
    },
    PSRAM: {
        PSRAM_SIZE: 2354,
        PSRAM_FREE: 2354,
        PSRAM_MIN: 2354
    },
    CHIP: {
        CHIP_MODEL: "ESP32-S2FH4",
        CHIP_CORES: "2",
        CHIP_SPEED: "80",
        PROG_SIZE: "2048",
    },
    CODE: {
        CODE_SIZE: 4096,
        CODE_FREE: 4096,
        FLASH_SIZE: 4096,
    },
    CPU: {
        CPU_USED: .8,
        CPU_USED_CORE0: .35,
        CPU_USED_CORE1: .45
    }
});

return <Box className={classes.root}>
    {Object.entries(statisics).map(entry=><Stat key={entry[0]} name={entry[0]} value={entry[1]} />)}
</Box>});

const MainApp = withStyles(mainAppStyle)(props => {
    const { classes } = props;
    const [drawerOpened, setDrawerOpened] = React.useState(false);
    const [mode, setMode] = React.useState('dark');
    const [stats, setStats] = React.useState(true);

    return <ThemeProvider theme={mode == "dark" ? darkTheme : lightTheme}>
        <CssBaseline />
        <Box className={classes.root}>
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
                    <IconButton>
                    <Badge aria-label="Alerts" badgeContent={4} color="secondary">
                        <Icon>notifications</Icon>
                    </Badge>
                    </IconButton>
                </Toolbar>
            </AppBar>
            <Drawer variant="permanent" 
                    classes={{paper: [classes.drawer, !drawerOpened && classes.drawerClosed].join(" ")}}>
                <div className={classes.drawerHeader}>
                    <Box className={classes.displayMode}>
                        <IconButton onClick={()=>setMode(mode === "dark" ? "light" : "dark")} ><Icon>{mode === "dark" ? "dark_mode" : "light_mode"}</Icon></IconButton>
                        <ListItemText primary={(mode === "dark" ? "Dark" : "Light") + " mode"}/>
                    </Box>
                    <IconButton onClick={()=>setDrawerOpened(!drawerOpened)}>
                        <Icon>chevron_left</Icon>
                    </IconButton>
                </div>
                <Divider/>
                <List>
                    <ListItem>
                        <ListItemIcon><IconButton onClick={() => setStats(!stats)}><Icon>area_chart</Icon></IconButton></ListItemIcon>
                        <ListItemText primary="Statistics"/>
                    </ListItem>
                    <ListItem>
                        <ListItemIcon><IconButton><Icon>design_services</Icon></IconButton></ListItemIcon>
                        <ListItemText primary="Effects Designer"/>
                    </ListItem>
                    <ListItem>
                        <ListItemIcon><IconButton><Icon>settings</Icon></IconButton></ListItemIcon>
                        <ListItemText primary="Settings"/>
                    </ListItem>
                </List>
            </Drawer>
            <Box className={[classes.content, drawerOpened && classes.contentShrinked].join(" ")}>
                {stats && <StatsPanel/>}
            </Box>
        </Box>
    </ThemeProvider>;
});
ReactDOM.createRoot(document.getElementById("root"))
        .render(<MainApp/>);
