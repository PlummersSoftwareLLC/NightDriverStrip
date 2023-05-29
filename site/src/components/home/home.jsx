const MainApp = () => {
    const [mode, setMode] = useState('dark');
    const theme = React.useMemo(
        () => getTheme(mode),[mode]);
    return <ThemeProvider theme={theme}>
            <CssBaseline />
            <Esp32 />
            <SiteConfig />
            <AppPannel mode={mode} setMode={setMode} />
           </ThemeProvider>
};

const AppPannel = withStyles(mainAppStyle)(props => {
    const { classes, mode, setMode } = props;
    const [drawerOpened, setDrawerOpened] = useState(false);
    const [stats, setStats] = useState(false);
    const [designer, setDesigner] = useState(false);

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
                        NightDriverStrip
                    </Typography>
                    <NotificationPanel/>
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
                        <ListItemIcon><IconButton onClick={() => item.setter(prevValue => !prevValue)}>
                            <Icon color="action" className={item.flag && classes.optionSelected}>{item.icon}</Icon>
                        </IconButton></ListItemIcon>
                        <ListItemText primary={item.caption}/>
                        {drawerOpened && (item.icon === "settings") && <ConfigPanel/>}
                    </ListItem>)
                }</List>
            </Drawer>
            <Box className={[classes.content, drawerOpened && classes.contentShrinked].join(" ")}>
                <StatsPanel open={stats}/> 
                <DesignerPanel open={designer}/>
            </Box>
        </Box>
});
