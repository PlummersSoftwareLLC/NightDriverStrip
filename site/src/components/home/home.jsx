const MainApp = withStyles(mainAppStyle)(props => {
    const { classes } = props;
    const [drawerOpened, setDrawerOpened] = React.useState(false);
    const [mode, setMode] = React.useState('dark');
    const [stats, setStats] = React.useState(true);
    const [designer, setDesigner] = React.useState(false);
    const [config, setConfig] = React.useState(false);
    const [statsRefreshRate, setStatsRefreshRate ] = React.useState(3);
    const [maxSamples, setMaxSamples ] = React.useState(50);
    const [animateChart, setAnimateChart ] = React.useState(false);

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
                    {
                        [{caption:"Statistics", flag: stats, setter: setStats, icon: "area_chart"},
                         {caption:"Effects Designer", flag: designer, setter: setDesigner, icon: "design_services"},
                         {caption:"Settings", flag: config, setter: setConfig, icon: "settings"}].map(item => 
                        <ListItem key={item.caption}>
                            <ListItemIcon><IconButton onClick={() => item.setter(!item.flag)}>
                                <Icon className={item.flag && (item.icon !== "settings") && classes.optionSelected}>{item.icon}</Icon>
                            </IconButton></ListItemIcon>
                            <ListItemText primary={item.caption}/>
                        </ListItem>)
                    }
                </List>
            </Drawer>
            <Box className={[classes.content, drawerOpened && classes.contentShrinked].join(" ")}>
                {stats && <StatsPanel siteConfig={siteConfig}/>}
                {config && <ConfigDialog siteConfig={siteConfig} onClose={()=>setConfig(false)} />}
            </Box>
        </Box>
    </ThemeProvider>;
});
