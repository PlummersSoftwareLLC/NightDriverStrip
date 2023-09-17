import {useState, useMemo, useEffect} from 'react';
import {AppBar, Toolbar, IconButton, Icon, Typography, Box} from '@mui/material'
import { CssBaseline, Drawer, Divider, List, ListItem, ListItemIcon, ListItemText } from '@mui/material'
import { ThemeProvider } from '@mui/material/styles';
import mainAppStyle from './style';
import getTheme from '../../theme/theme';
import NotificationPanel from './notifications/notifications';
import ConfigPanel from './config/config';
import StatsPanel from './statistics/stats';
import DesignerPanel from './designer/designer';
import { useTheme } from '@emotion/react';
import PropTypes from 'prop-types';

const MainApp = () => {
    const [mode, setMode] = useState(localStorage.getItem('theme') || 'dark');
    const theme = useMemo(
        () => getTheme(mode),[mode]);

    // save users state to storage so the page reloads where they left off. 
    useEffect(() => {
        localStorage.setItem('theme', mode);
    }, [mode])

    return <ThemeProvider theme={theme}><CssBaseline /><AppPannel mode={mode} setMode={setMode} /></ThemeProvider>
};
const AppPannel = (props) => {
    const config = JSON.parse(localStorage.getItem('config'));
    const { mode, setMode } = props;
    const theme = useTheme();
    const classes = mainAppStyle(theme);
    const [drawerOpened, setDrawerOpened] = useState(config && config.drawerOpened !== undefined ? config.drawerOpened : false);
    const [stats, setStats] = useState(config && config.stats !== undefined ? config.stats : true);
    const [designer, setDesigner] = useState(config && config.designer !== undefined ? config.designer : true);
    const [statsRefreshRate, setStatsRefreshRate ] = useState(config && config.statsRefreshRate !== undefined ? config.statsRefreshRate : 3);
    const [maxSamples, setMaxSamples ] = useState(config && config.maxSamples !== undefined ? config.maxSamples : 50);
    const [animateChart, setAnimateChart ] = useState(config && config.animateChart !== undefined ? config.animateChart : false);
    const [notifications, setNotifications] = useState([]);
    
    // save users state to storage so the page reloads where they left off. 
    useEffect(() => {
        localStorage.setItem('config', JSON.stringify({
            stats,
            designer,
            statsRefreshRate,
            animateChart,
            maxSamples 
        }));
    }, [stats, designer, statsRefreshRate, animateChart, maxSamples])

    
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
    const rootClasses = drawerOpened ? classes.appbarOpened : classes.appbarClosed;
    const drawerClosedClasses = drawerOpened ? {} : classes.drawerClosed;
    return <Box >
        <AppBar sx={{...classes.appbar, ...rootClasses}} >
            <Toolbar>
                <IconButton 
                    aria-label="Open drawer" 
                    onClick={()=>setDrawerOpened(!drawerOpened)} 
                >
                    <Icon>{drawerOpened ? "chevron" : "menu"}</Icon>
                </IconButton>
                <Typography
                    sx={classes.toolbarTitle}
                    component="h1"
                    variant="h6">
                        NightDriverStrip
                </Typography>
                {(notifications.length > 0) && <NotificationPanel notifications={notifications} clearNotifications={()=>setNotifications([])}/>}
            </Toolbar>
        </AppBar>
        <Drawer
            open={drawerOpened}
            variant="permanent"
            sx={{'& .MuiDrawer-paper': {...classes.drawer, ...drawerClosedClasses}}}
        >
            <Box sx={{...classes.drawerHeader}}>
                <Box sx={classes.displayMode}>
                    <IconButton onClick={()=>setMode(mode === "dark" ? "light" : "dark")} ><Icon>{mode === "dark" ? "dark_mode" : "light_mode"}</Icon></IconButton>
                    <ListItemText primary={(mode === "dark" ? "Dark" : "Light") + " mode"}/>
                </Box>
                <IconButton onClick={()=>setDrawerOpened(!drawerOpened)}>
                    <Icon>chevron_left</Icon>
                </IconButton>
            </Box> 
            <Divider/>
            <List>{
                [
                    {caption:"Home", flag: designer, setter: setDesigner, icon: "home"},
                    {caption:"Statistics", flag: stats, setter: setStats, icon: "area_chart"},
                    {caption:"", flag: drawerOpened, icon: "settings", setter: setDrawerOpened}
                ].map(item => 
                    <ListItem key={item.icon}>
                        <ListItemIcon><IconButton onClick={() => item.setter && item.setter(prevValue => !prevValue)}>
                            <Icon color={item.flag ? "primary": "action"} >{item.icon}</Icon>
                        </IconButton></ListItemIcon>
                        <ListItemText primary={item.caption}/>
                        {drawerOpened && (item.icon === "settings") && <ConfigPanel siteConfig={siteConfig} />}
                    </ListItem>)
            }</List>
        </Drawer>
        <Box
            sx={{...classes.content,
                p: 10,
                pl: drawerOpened ? 30: 10}}>
            <StatsPanel siteConfig={siteConfig} open={stats} addNotification={addNotification}/> 
            <DesignerPanel siteConfig={siteConfig} open={designer} addNotification={addNotification}/>
        </Box>
    </Box>
};

AppPannel.propTypes = {
    mode: PropTypes.string.isRequired, 
    setMode: PropTypes.func.isRequired
}

export default MainApp;