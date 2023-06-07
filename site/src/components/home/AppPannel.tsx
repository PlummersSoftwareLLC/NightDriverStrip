import { Box, AppBar, Divider, Drawer, Icon, IconButton, List, ListItem, ListItemIcon, ListItemText, Toolbar, Typography, ThemeProvider, CssBaseline } from "@mui/material";
import { useEffect, useState } from "react";
import { withStyles } from "tss-react/mui";
import { useThemeSwitcher } from "./ThemeSwitcherProvider";
import { NotificationPanel } from "./notifications/notifications";
import { ConfigPanel } from "../config/config";
import { StatsPanel } from "./statistics/stats";
import { DesignerPanel } from "./designer/designer";
import { httpPrefix } from "../../espaddr";
import { eventManager } from "../../services/eventManager/eventmanager";
import { SiteConfig } from "../../config/siteconfig";
import { DevicePicker } from "./devicepicker/devicepicker";
import { mainAppStyle } from "./style"

enum DrawerWidth {
    close=80,
    open=300
}

const toolbarHeight=74;

export const AppPannel = withStyles(({classes}) => {
    const { theme, themeMode, setThemeMode } = useThemeSwitcher();
    const [ drawerWidth, setDrawerWidth ] = useState(DrawerWidth.close);
    const [drawerOpened, setDrawerOpened] = useState(false);
    const [stats, setStats] = useState(false);
    const [designer, setDesigner] = useState(true);
    const [activeDevice, setActiveDevice] = useState(httpPrefix||"Current Device");
    const [smallScreen, setSmallScreen ] = useState(false);
    const [service] = useState(eventManager());

    useEffect(()=>{
        const subs = {
            isSmallScreen: service.subscribe("isSmallScreen",setSmallScreen),
            config: service.subscribe("SiteConfig",cfg => setThemeMode(cfg.UIMode.value))
        };
        return ()=>Object.values(subs).forEach(sub=>service.unsubscribe(sub));
    },[service]);

    useEffect(()=>setDrawerWidth(drawerOpened?DrawerWidth.open:DrawerWidth.close),[drawerOpened]);
    
    return <ThemeProvider theme={theme}>
        <CssBaseline />
            <SiteConfig />
            <AppBar sx={{
                zIndex: theme.zIndex.drawer + 1,
                transition: theme.transitions.create(['width', 'margin'], {
                    easing: theme.transitions.easing.sharp,
                    duration: theme.transitions.duration.leavingScreen,
                }),
                ...(drawerOpened && {
                    marginLeft: drawerWidth,
                    width: `calc(100% - ${drawerWidth}px)`,
                    transition: theme.transitions.create(['width', 'margin'], {
                        easing: theme.transitions.easing.sharp,
                        duration: theme.transitions.duration.enteringScreen,
                    })
                }),
            }}>
                <Toolbar>
                    <IconButton 
                        aria-label="Open drawer" 
                        className={drawerOpened && classes.drawerClosed}
                        onClick={()=>setDrawerOpened(!drawerOpened)} 
                        sx={{
                            ...(drawerOpened && {
                                overflowX: "hidden",
                                transition: theme.transitions.create('width', {
                                    easing: theme.transitions.easing.sharp,
                                    duration: theme.transitions.duration.leavingScreen
                                })
                            })
                        }}>
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
                    sx={{
                        whiteSpace: 'nowrap',
                        width: DrawerWidth.open,
                        zIndex: 0,
                        transition: theme.transitions.create('width', {
                            easing: theme.transitions.easing.sharp,
                            duration: theme.transitions.duration.enteringScreen,
                        })
                    }}>
                {drawerOpened?<Box sx={{width:DrawerWidth.open, display:"flex", justifyContent: "space-between", alignItems: "center"}}>
                    <Box sx={{display:"flex",height:toolbarHeight, alignItems: "center"}}>
                        <IconButton aria-label="Display Mode" onClick={()=>service.emit("SetSiteConfigItem",{value:themeMode==="dark"?"light":"dark", id:"UIMode"})} ><Icon>{themeMode === "dark" ? "dark_mode" : "light_mode"}</Icon></IconButton>
                        <ListItemText primary={(themeMode === "dark" ? "Dark" : "Light") + " Mode"}/>
                    </Box>
                    <IconButton aria-label="Close Config" onClick={()=>setDrawerOpened(!drawerOpened)}>
                        <Icon>chevron_left</Icon>
                    </IconButton>
                </Box>:<Box sx={{height:toolbarHeight}}></Box>}
                <Divider/>
                <List>{[
                    [{caption:"Home", flag: designer, setter: setDesigner, icon: "home"},
                     {caption:"Statistics", flag: stats, setter: setStats, icon: "area_chart"},
                     {caption: "Configuration", flag: drawerOpened, icon: "settings", setter: setDrawerOpened}].map(item =>
                    <ListItem key={item.icon}>
                        <ListItemIcon><IconButton aria-label={item.caption} onClick={() => item.setter(prevValue => !prevValue)}>
                            <Icon color="action">{item.icon}</Icon>
                        </IconButton></ListItemIcon>
                    </ListItem>),
                    drawerOpened && <ListItem key="setting">
                        {!drawerOpened && <ListItemIcon><IconButton aria-label="Configuration" onClick={() => setDrawerOpened(prevValue => !prevValue)}>
                            <Icon color="action">config</Icon>
                        </IconButton></ListItemIcon>}
                        {drawerOpened &&<ConfigPanel />}
                    </ListItem>]}
                </List>
            </Drawer>
            <Box sx={{
                paddingLeft: `${DrawerWidth.close}px`,
                paddingTop: `${toolbarHeight}px`,
                transition: theme.transitions.create('padding-left', {
                    easing: theme.transitions.easing.sharp,
                    duration: theme.transitions.duration.leavingScreen,
                }),
                display: "flex",
                flexDirection: "column",
                flexWrap: "wrap",
                rowGap: "10px",
                ...(drawerOpened && {
                    paddingLeft: drawerWidth + 10,
                    transition: theme.transitions.create('padding-left', {
                        easing: theme.transitions.easing.sharp,
                        duration: theme.transitions.duration.leavingScreen,
                    })
                })
            }}>
                <StatsPanel open={stats} smallScreen={smallScreen} /> 
                <DesignerPanel open={designer} displayMode="detailed"/>
            </Box>
    </ThemeProvider>
},theme=>mainAppStyle(theme,{closeDrawerWidth:DrawerWidth.close,openDrawerWidth:DrawerWidth.open,toolbarHeight}));
