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
