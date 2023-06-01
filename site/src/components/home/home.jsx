const MainApp = () => {
    return <AppPannel/>
};

const AppPannel = withStyles(mainAppStyle)(props => {
    const {classes} = props;
    const [drawerOpened, setDrawerOpened] = useState(false);
    const [stats, setStats] = useState(false);
    const [designer, setDesigner] = useState(false);
    const [mode, setMode] = useState('dark');
    const [activeDevice, setActiveDevice] = useState(httpPrefix);
    const theme = React.useMemo(() => getTheme(mode),[mode]);

    return <ThemeProvider theme={theme}>
        <CssBaseline />
        <Box className={classes.root}>
            <Esp32 activeHttpPrefix={activeDevice} />
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
                    <NotificationPanel/>
                </Toolbar>
            </AppBar>
            <Drawer variant="permanent" 
                    classes={{paper: [classes.drawer, !drawerOpened && classes.drawerClosed].join(" ")}}>
                <Box className={classes.drawerHeader}>
                    <Box className={classes.displayMode}>
                        <IconButton aria-label="Display Mode" onClick={()=>setMode(mode === "dark" ? "light" : "dark")} ><Icon>{mode === "dark" ? "dark_mode" : "light_mode"}</Icon></IconButton>
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
            <Box className={[classes.content, drawerOpened && classes.contentShrinked].join(" ")}>
                <StatsPanel open={stats}/> 
                <DesignerPanel open={designer}/>
            </Box>
        </Box>
    </ThemeProvider>
});
