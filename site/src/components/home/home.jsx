const MainApp = withStyles(mainAppStyle)(props => {
    const { classes } = props;
    const [drawerOpened, setDrawerOpened] = useState(false);
    const [mode, setMode] = useState('dark');
    const [stats, setStats] = useState(false);
    const [designer, setDesigner] = useState(true);
    const [config, setConfig] = useState(false);
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
            name: "Animate chart changes",
            value: animateChart,
            setter: setAnimateChart,
            type: "boolean"
        },
        maxSamples: {
            name: "Number of chart points",
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
                    <NotificationPanel notifications={notifications}/>
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
                    [{caption:"Statistics", flag: stats, setter: setStats, icon: "area_chart"},
                     {caption:"Effects Designer", flag: designer, setter: setDesigner, icon: "design_services"},
                     {caption:"Settings", flag: config, setter: setConfig, icon: "settings"}].map(item => 
                    <ListItem key={item.caption}>
                        <ListItemIcon><IconButton onClick={() => item.setter(prevValue => !prevValue)}>
                            <Icon className={item.flag && (item.icon !== "settings") && classes.optionSelected}>{item.icon}</Icon>
                        </IconButton></ListItemIcon>
                        <ListItemText primary={item.caption}/>
                    </ListItem>)
                }</List>
            </Drawer>
            <Box className={[classes.content, drawerOpened && classes.contentShrinked].join(" ")}>
                <StatsPanel siteConfig={siteConfig} open={stats} addNotification={addNotification}/> 
                <DesignerPanel siteConfig={siteConfig} open={designer} addNotification={addNotification}/>
                <ConfigDialog siteConfig={siteConfig} open={config} addNotification={addNotification} onClose={() => {setConfig(false)}} />
            </Box>
        </Box>
    </ThemeProvider>;
});
