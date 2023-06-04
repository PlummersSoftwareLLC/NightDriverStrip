const MainApp = () => {
    return <AppPannel/>
};

const AppPannel = withStyles(mainAppStyle)(props => {
    const {classes} = props;
    const [drawerOpened, setDrawerOpened] = useState(false);
    const [stats, setStats] = useState(false);
    const [designer, setDesigner] = useState(true);
    const [mode, setMode] = useState('dark');
    const [activeDevice, setActiveDevice] = useState(httpPrefix||"Current Device");
    const [smallScreen, setSmallScreen ] = useState(false);
    const [service] = useState(eventManager());
    const theme = React.useMemo(() => getTheme(mode),[mode]);

    useEffect(()=>{
        const subs = {
            isSmallScreen: service.subscribe("isSmallScreen",setSmallScreen),
            config: service.subscribe("SiteConfig",cfg => setMode(cfg.UIMode.value))
        };
        return ()=>Object.value(subs).forEach(sub=>service.unsubscribe(sub));
    },[service]);
    
    return <ThemeProvider theme={theme}>
        <CssBaseline />
        <Box className={classes.root}>
            <SiteConfig />
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
                        NightDriverStrip
                    </Typography>
                    <DevicePicker {...{activeDevice, setActiveDevice}} />
                    <Icon>{smallScreen ?"smartphone":"computer"}</Icon>
                    <NotificationPanel/>
                </Toolbar>
            </AppBar>
            <Drawer variant="permanent" 
                    classes={{paper: [classes.drawer, !drawerOpened && classes.drawerClosed].join(" ")}}>
                <Box className={classes.drawerHeader}>
                    <Box className={classes.displayMode}>
                        <IconButton aria-label="Display Mode" onClick={()=>service.emit("SetSiteConfigItem",{value:mode==="dark"?"light":"dark", id:"UIMode"})} ><Icon>{mode === "dark" ? "dark_mode" : "light_mode"}</Icon></IconButton>
                        <ListItemText primary={(mode === "dark" ? "Dark" : "Light") + " mode"}/>
                    </Box>
                    <IconButton aria-label="Close Config" onClick={()=>setDrawerOpened(!drawerOpened)}>
                        <Icon>chevron_left</Icon>
                    </IconButton>
                </Box>
                <Divider/>
                <List className={classes.setting}>{[
                    [{caption:"Home", flag: designer, setter: setDesigner, icon: "home"},
                     {caption:"Statistics", flag: stats, setter: setStats, icon: "area_chart"},
                     {caption: "Configuration", flag: drawerOpened, icon: "settings", setter: setDrawerOpened}].map(item =>
                    <ListItem className={classes.settingItem} key={item.icon}>
                        <ListItemIcon><IconButton aria-label={item.caption} onClick={() => item.setter(prevValue => !prevValue)}>
                            <Icon color="action" className={item.flag ? classes.optionSelected : classes.optionUnselected}>{item.icon}</Icon>
                        </IconButton></ListItemIcon>
                    </ListItem>),
                    drawerOpened && <ListItem key="setting">
                        {!drawerOpened && <ListItemIcon><IconButton aria-label="Configuration" onClick={() => setDrawerOpened(prevValue => !prevValue)}>
                            <Icon color="action" className={drawerOpened ? classes.optionSelected : classes.optionUnselected}>config</Icon>
                        </IconButton></ListItemIcon>}
                        {drawerOpened &&<ConfigPanel/>}
                    </ListItem>]}
                </List>
            </Drawer>
            <Box className={[classes.content, drawerOpened ? classes.contentShrinked:"",smallScreen?classes.smallScreen:""].join(" ")}>
                <StatsPanel open={stats} smallScreen={smallScreen} /> 
                <DesignerPanel open={designer} displayMode="detailed"/>
            </Box>
        </Box>
    </ThemeProvider>
});
