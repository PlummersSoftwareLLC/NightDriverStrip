import {useState, useMemo} from 'react';
import {AppBar, Toolbar, IconButton, Icon, Typography, Box} from '@mui/material'
import { CssBaseline, Drawer, Divider, List, ListItem, ListItemIcon, ListItemText } from '@mui/material'
import { ThemeProvider } from '@mui/material/styles';
import {withStyles} from '@mui/styles';
import mainAppStyle from './style';
import getTheme from '../../theme/theme';
import NotificationPanel from './notifications/notifications';
import ConfigPanel from './config/config';
import StatsPanel from './statistics/stats';
import DesignerPanel from './designer/designer';

const MainApp = () => {
    const [mode, setMode] = useState('dark');
    const theme = useMemo(
        () => getTheme(mode),[mode]);
    return <ThemeProvider theme={theme}><CssBaseline /><AppPannel mode={mode} setMode={setMode} /></ThemeProvider>
};

const AppPannel = withStyles(mainAppStyle)(props => {
    const { classes, mode, setMode } = props;
    const [drawerOpened, setDrawerOpened] = useState(false);
    const [settingsOpened, setSettingsOpened] = useState(false);
    const [stats, setStats] = useState(false);
    const [designer, setDesigner] = useState(true);
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
                    className={drawerOpened ? classes.drawerClosed : ""}>
                    <Icon>{drawerOpened ? "chevron" : "menu"}</Icon>
                </IconButton>
                <Typography
                    className={classes.toolbarTitle}
                    component="h1"
                    variant="h6">
                        NightDriverStrip
                </Typography>
                {(notifications.length > 0) && <NotificationPanel notifications={notifications} clearNotifications={()=>setNotifications([])}/>}
            </Toolbar>
        </AppBar>
        <Drawer
            open={drawerOpened}
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
                    {caption:"", flag: settingsOpened, icon: "settings", setter: setSettingsOpened}].map(item => 
                    <ListItem key={item.icon}>
                        <ListItemIcon><IconButton onClick={() => item.setter && item.setter(prevValue => !prevValue)}>
                            <Icon color="action" className={item.flag ? classes.optionSelected: ""}>{item.icon}</Icon>
                        </IconButton></ListItemIcon>
                        <ListItemText primary={item.caption}/>
                        {settingsOpened && (item.icon === "settings") && <ConfigPanel siteConfig={siteConfig} />}
                    </ListItem>)
            }</List>
        </Drawer>
        <Box className={[classes.content, drawerOpened && classes.contentShrinked].join(" ")}>
            <StatsPanel siteConfig={siteConfig} open={stats} addNotification={addNotification}/> 
            <DesignerPanel siteConfig={siteConfig} open={designer} addNotification={addNotification}/>
        </Box>
    </Box>
});

export default MainApp;