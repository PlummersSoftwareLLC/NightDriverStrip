const httpPrefix='';
const { useState, useEffect, useMemo, useRef } = window.React;

const { createTheme, ThemeProvider, Checkbox, AppBar, Toolbar, IconButton, Icon, MenuIcon, Typography } = window.MaterialUI;
const { Badge, withStyles, CssBaseline, Drawer, Divider, List, ListItem, ListItemIcon, ListItemText } = window.MaterialUI;
const { Box, Dialog, Slide, Button, TextField, FormControlLabel, useTheme } = window.MaterialUI;

const { AreaChart, BarChart, Area, Bar, ResponsiveContainer, LineChart, Line, CartesianGrid, XAxis, YAxis, Tooltip, Legend } = window.Recharts;

const typography={
  littleHeader: {
    color: 'red'
  },
  littleValue: {
    lineHeight: 1.0,
    fontSize: "3.75rem",
    fontWeight: 100
  }
};
    
const lightTheme = createTheme({
    palette: {
      mode: 'light',
      type: 'light',
      taskManager: {
        strokeColor: '#90ff91',
        MemoryColor: '#0002ff',
        idleColor: 'black',
        color1: '#58be59db',
        color2: '#58be59a1',
        color3: '#58be596b',
        color4: '#58be5921',
        bcolor1: '#189cdbff',
        bcolor2: '#189cdba1',
        bcolor3: '#189cdb66',
        bcolor4: '#189cdb38',
      }
      },
    typography
});

const darkTheme = createTheme({
  palette: {
    mode: 'dark',
    type: 'dark',
    taskManager: {
      strokeColor: '#90ff91',
      MemoryColor: '#0002ff',
      idleColor: 'black',
      color1: '#58be59db',
      color2: '#58be59a1',
      color3: '#58be596b',
      color4: '#58be5921',
      bcolor1: '#189cdbff',
      bcolor2: '#189cdba1',
      bcolor3: '#189cdb66',
      bcolor4: '#189cdb38',
    }
  },
  typography
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
    categoryStats: {
        "display": "flex",
        "flex-direction": "row",
        "flex-wrap": "wrap",
        "align-content": "flex-start",
        "justify-content": "center",
        "align-items": "center"
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
    },
    optionSelected: {
      color: "aquamarine"
    }
  });
  const configStyle = theme => ({
    configBar: {
        "padding-top": "65px"
    },
    configDisplay: {
        "display": "flex",
        "column-gap": "10px",
        "flex-direction": "row",
        "flex-wrap": "nowrap",
        "justify-content": "flex-start",
        "align-items": "center",
    },
    saveIcons: {
        display: "flex",
        "flex-direction": "column"
    },
    cblabel: {
        "margin-left": "initial"
    },
    hidden: {
        display: "none"
    }
});const statsStyle = theme => ({
    root: {
        "display": "flex",
        "flex-direction": "row",
        "flex-wrap": "wrap",
        "align-content": "flex-start",
        "justify-content": "flex-start",
        "align-items": "flex-start"
    },
    chartArea: {
        "display": "flex",
        "flex-direction": "row",
        "flex-wrap": "wrap",
        "align-content": "flex-start",
        "justify-content": "flex-start",
        "align-items": "flex-start"
    },
    categoryStats:{
        "display": "flex",
        "flex-direction": "row",
        "flex-wrap": "wrap",
        "align-content": "flex-start",
        "justify-content": "space-between",
        "align-items": "flex-start"
    },
    hidden: {
        display: "none"
    },
    statCatergoryHeader: {
        "display": "flex",
        "flex-direction": "row",
        "flex-wrap": "wrap",
        "align-content": "flex-start",
        "justify-content": "center",
        "align-items": "center"
    }
});

const statStyle = theme => ({
    root: {
      display: 'flex',
      "flex-direction": "column"
    }
});
const staticStatStyle = theme => ({
    root: {
        "display": "flex",
        "flex-direction": "row",
        "flex-wrap": "wrap",
        "align-content": "flex-start",
        "justify-content": "space-between",
        "align-items": "flex-start"
    },
    attribute: {
        "color": theme.palette.text.secondary
    }
});
const areaChartStyle = theme => ({
    root: {
        "display": "flex",
        "flex-direction": "column",
        "flex-wrap": "wrap",
        "align-content": "flex-start",
        "justify-content": "flex-start",
        "align-items": "stretch"
    },
    header: {
        "display": "flex",
        "flex-direction": "column",
        "flex-wrap": "nowrap",
        "justify-content": "center",
        "align-items": "stretch"
    },
    headerLine: {
        "display": "flex",
        "flex-direction": "row",
        "flex-wrap": "nowrap",
        "justify-content": "space-between",
        "align-items": "center",
        "width": "100%",
        "color": theme.palette.text.primary
    },
    headerField: {
        "display": "flex",
        "flex-direction": "row",
        "flex-wrap": "nowrap",
        "justify-content": "center",
        "align-items": "center",
        "width": "inherit",
        "color": theme.palette.text.secondary
    },
    stats: {
        "display": "flex",
        "flex-direction": "row",
        "flex-wrap": "nowrap",
        "justify-content": "center",
        "align-items": "center",
        "padding": "0px",
        "color": theme.palette.text.secondary
    },
    stat: {
        "display": "flex",
        "padding": "0px",
        "flex-wrap": "nowrap",
        "align-items": "center",
        "flex-direction": "row",
        "justify-content": "center"
    },
    tooltipContent: {
        "display": "flex",
        "flex-direction": "column",
        "flex-wrap": "nowrap",
        "align-content": "center",
        "justify-content": "center",
        "align-items": "stretch",
        "background-color": "black",
        "padding": "5px"
    },
    tooltipHeader: {
        "font-size": "medium",
        "display": "flex",
        "flex-direction": "column",
        "flex-wrap": "nowrap",
        "align-content": "center",
        "align-items": "center",
        "justify-content": "center",
        "border-bottom": "solid 1px"
    },
    threads: {
        "margin": "0px",
        "padding": "0px",
        "display": "flex",
        "flex-direction": "column",
        "flex-wrap": "nowrap",
        "align-content": "center",
        "justify-content": "center",
        "align-items": "stretch",
        "font-size": "small",
    },
    thread: {
        "display": "flex",
        "flex-direction": "row",
        "flex-wrap": "nowrap",
        "align-content": "center",
        "justify-content": "space-between",
        "align-items": "center",
        "column-gap": "5px",
    },
    threadValue: {
        "display": "flex",
        "flex-direction": "row",
        "flex-wrap": "nowrap",
        "align-content": "center",
        "justify-content": "center",
        "align-items": "center",
        "font-size": "smaller",
        "color": "aquamarine",
        "column-gap": "3px"
    },
    threadSummary: {
        "font-size": "x-small",
        "color": "aqua"
    }
});
const barChartStyle = theme => ({
    summary: {
        "display": "flex",
        "flex-direction": "column",
        "flex-wrap": "wrap",
        "align-content": "flex-start",
        "justify-content": "flex-start",
        "align-items": "center"
    },
    minimized: {
        "margin-top":"-20px",
        "margin-right": "-25px"
    },
    header: {
        "display": "flex",
        "flex-direction": "column",
        "flex-wrap": "nowrap",
        "justify-content": "center",
        "align-items": "stretch"
    },
    stats: {
        "display": "flex",
        "flex-direction": "row",
        "flex-wrap": "nowrap",
        "justify-content": "center",
        "align-items": "center",
        "padding": "0px"
    },
    stat: {
        "display": "flex",
        "padding": "0px",
        "flex-wrap": "nowrap",
        "align-items": "center",
        "flex-direction": "row",
        "justify-content": "center"
    },
    tooltipContent: {
        "display": "flex",
        "flex-direction": "column",
        "flex-wrap": "nowrap",
        "align-content": "center",
        "justify-content": "center",
        "align-items": "stretch",
        "background-color": "black",
        "padding": "5px"
    },
    tooltipHeader: {
        "font-size": "medium",
        "display": "flex",
        "flex-direction": "column",
        "flex-wrap": "nowrap",
        "align-content": "center",
        "align-items": "center",
        "justify-content": "center",
        "border-bottom": "solid 1px"
    },
    threads: {
        "margin": "0px",
        "padding": "0px",
        "display": "flex",
        "flex-direction": "column",
        "flex-wrap": "nowrap",
        "align-content": "center",
        "justify-content": "center",
        "align-items": "stretch",
        "font-size": "small",
    },
    thread: {
        "display": "flex",
        "flex-direction": "row",
        "flex-wrap": "nowrap",
        "align-content": "center",
        "justify-content": "space-between",
        "align-items": "center",
        "column-gap": "5px",
    },
    threadValue: {
        "display": "flex",
        "flex-direction": "row",
        "flex-wrap": "nowrap",
        "align-content": "center",
        "justify-content": "center",
        "align-items": "center",
        "font-size": "smaller",
        "color": "aquamarine",
        "column-gap": "3px"
    },
    threadSummary: {
        "font-size": "x-small",
        "color": "aqua"
    }
});
const MainApp = withStyles(mainAppStyle)(props => {
    const { classes } = props;
    const [drawerOpened, setDrawerOpened] = useState(false);
    const [mode, setMode] = useState('dark');
    const [stats, setStats] = useState(true);
    const [designer, setDesigner] = useState(false);
    const [config, setConfig] = useState(false);
    const [statsRefreshRate, setStatsRefreshRate ] = useState(3);
    const [maxSamples, setMaxSamples ] = useState(50);
    const [animateChart, setAnimateChart ] = useState(false);

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
                <StatsPanel siteConfig={siteConfig} open={stats} />
                <ConfigDialog siteConfig={siteConfig} open={config} onClose={() => {setConfig(false)}} />
            </Box>
        </Box>
    </ThemeProvider>;
});
const ConfigItem = withStyles(configStyle)(props => {
    const { name, value, configItemUpdated, datatype, classes } = props;
    const [ editing, setEditing] = useState(false);
    const [ configValue, setConfigValue] = useState(value);
    const getConfigValue = (value, type) => {
        switch (type) {
            case "int":
                return parseInt(value)
            case "float":
                return parseFloat(value)
            default:
                return value;
        }
    }   

    if (datatype === "boolean") {
        return <ListItem button onClick={_evt=>!editing && setEditing(!editing)}>
            <FormControlLabel
                className={classes.cblabel}
                label={name} 
                labelPlacement="start"
                control={<Checkbox 
                    defaultChecked={value}
                    onChange={event => {
                        setConfigValue(event.target.checked);
                        configItemUpdated(event.target.checked);
                    }} />} />
        </ListItem>;
    }

    return <ListItem button onClick={_evt=>!editing && setEditing(!editing)}>
                {!editing && <ListItemText className={ classes.configDisplay }
                    primary={name}
                    secondary={configValue}/>}
                {editing && <TextField label={name} 
                                       variant="outlined"
                                       type={["int","float"].includes(datatype) ? "number" : "text"}
                                       pattern={datatype === "int" ? "^[0-9]+$" : (datatype === "float" ? "^[0-9]+[.0-9]*$" : ".*")}
                                       defaultValue={value}
                                       onChange={event => setConfigValue(getConfigValue(event.target.value,datatype)) } />}
                    <Box className={classes.saveIcons}>
                        {editing && <IconButton color="primary" 
                                            aria-label="Save" 
                                            component="label"
                                            onClick={_evt => {
                                                configItemUpdated(configValue)
                                                setEditing(false);
                                            }}>
                                    <Icon>save</Icon>
                                </IconButton>}
                        {editing && <IconButton color="primary" 
                                                aria-label="Cancel" 
                                                component="label"
                                                onClick={_evt => {
                                                    setConfigValue(value);
                                                    setEditing(false);
                                                }}>
                                        <Icon>cancel</Icon>
                                    </IconButton>}
                    </Box>
            </ListItem>;
});
const ConfigDialog = withStyles(configStyle)(props => {
  const { classes, open, onClose, siteConfig } = props;
  return (
    <Dialog
      fullScreen
      open={open}
      onClose={() => onClose && onClose()}>
      <AppBar sx={{ position: 'relative' }}>
        <Toolbar>
          <IconButton
            edge="start"
            color="inherit"
            onClick={()=>onClose && onClose()}
            aria-label="close">
            <Icon>close</Icon>
          </IconButton>
          <Typography sx={{ ml: 2, flex: 1 }} variant="h6" component="div">
            Configuration
          </Typography>
        </Toolbar>
      </AppBar>
      <List className={classes.configBar}>
        <ListItem>
          <List>
            <ListItemText primary="Site Configuration"/>
            <List>
                {Object.entries(siteConfig).map(entry => <ConfigItem 
                                            name={entry[1].name}
                                            key={entry[1].name}
                                            datatype={entry[1].type}
                                            value={entry[1].value}
                                            configItemUpdated={value => entry[1].setter(value)} 
                                            />)}
            </List>
          </List>
        </ListItem>
        <Divider />
      </List>
    </Dialog>
  );
});const StaticStatsPanel = withStyles(staticStatStyle)(props => {
    const { classes, stat, name, detail } = props;

    return <Box className={classes.root}>
        <Typography variant={detail ? "h5" : "h6"}>{name}</Typography>
        {detail && <List>
            {Object.entries(stat.stat)
                   .map(entry=>
                <ListItem key={entry[0]}>
                    <Typography variant="littleHeader">{entry[0]}</Typography>:
                    <Typography className={classes.attribute} variant="littleValue" >{entry[1]}</Typography>
                </ListItem>)}
        </List>}
    </Box>
});const AreaStat = withStyles(areaChartStyle)(props => {
    const { classes, name, rawvalue, ignored, statsAnimateChange, maxSamples, headerFields , idleField, category, detail } = props;
    const getChartValues = (value) => Object.entries(value)
                        .filter(entry=>!ignored.includes(entry[0]))
                        .reduce((ret,entry)=>{ret[entry[0]] = entry[1]; return ret},{});
    const [lastStates, setLastStates] = useState([Object.entries(getChartValues(rawvalue))
        .filter(entry=>!ignored.includes(entry[0]))
        .reduce((ret,stat)=>{ret[stat[0]]=stat[1]; return ret},{ts: new Date().getTime()})] );
    const getValue = (value) => value !== undefined && !Number.isInteger(value) ? (isNaN(value) ? value : value.toFixed(2)) : value;
    const theme = useTheme();

    useMemo(()=>{
        setLastStates(lastStates === undefined ? [Object.entries(getChartValues(rawvalue))] : [...lastStates,Object.entries(getChartValues(rawvalue))
            .filter(entry=>!ignored.includes(entry[0]))
            .reduce((ret,stat)=>{ret[stat[0]]=stat[1]; return ret},{ts: new Date().getTime()})]
            .filter((_val,idx,arr) => arr.length >= maxSamples ? idx > arr.length - maxSamples : true));
    },[rawvalue]);
    
    const getFillColor = ({step, isIdle}) => {
        if (isIdle) {
            return theme.palette.taskManager.idleColor;
        }
        return (theme.palette.taskManager[`${category === "Memory" ? "b" : ""}color${step+1}`]);
    }

    const getStatTooltip = (data, classes) => {
        return (
        <div className={classes.tooltipContent}>
            <div className={classes.tooltipHeader}>{data.labelFormatter(data.label)}</div>
            <ul className={classes.threads}>
                {data.payload
                    .sort((a,b) => sortStats(b,a))
                    .map(stat => 
                    <div key={stat.name} className={classes.thread}>
                        <div className={classes.threadName} style={{color:stat.color}}>{stat.name}</div>
                        <div className={classes.threadValue}>{getValue(stat.value)}
                            <div className={classes.threadSummary}>
                                ({(stat.value/data.payload.reduce((ret,stat) => ret + stat.value,0)*100).toFixed(2)}%)
                            </div>
                        </div>
                    </div>)
                }
            </ul>
        </div>)
    }

    return <Box className={classes.root}>
        {detail && <Box className={classes.header}>
            <Typography className={classes.headerLine} variant="subtitle1">{name} {headerFields && Object.values(headerFields).map(headerField=>
                <Typography key={headerField} className={classes.headerField} variant="subtitle2">{`${headerField}: ${rawvalue[headerField]}`}</Typography>)}
            </Typography>
            <List className={classes.stats}>
                {Object.entries(rawvalue)
                        .filter(entry=>!ignored.includes(entry[0]))
                        .map(entry=>
                    <ListItem className={classes.stats} key={entry[0]}>
                        <Typography variant="littleHeader">{entry[0]}</Typography>:
                        <Typography variant="littleValue" >{getValue(entry[1])}</Typography>
                    </ListItem>)}
            </List>
        </Box>}
        <AreaChart 
            data={lastStates}
            height={detail ? 300 : 80}
            width={detail ? 500 : 200}
            stackOffset="expand">
            <defs>
                {Object.entries(getChartValues(rawvalue))
                       .filter(entry => entry[1] !== undefined)
                       .map((entry,idx,arr) => <linearGradient key={`color${entry[0]}`} id={`color${entry[0]}`} x1="0" y1="0" x2="0" y2="1">
                                                <stop offset="5%" stopColor={getFillColor({numOfSteps: arr.length, step: idx, isIdle: entry[0] === idleField})} stopOpacity={0.8}/>
                                                <stop offset="95%" stopColor={getFillColor({numOfSteps: arr.length, step: idx, isIdle: entry[0] === idleField})} stopOpacity={0}/>
                                              </linearGradient>)}
            </defs>
            <XAxis dataKey="ts"
                   name='Time'
                   hide={!detail}
                   tickFormatter={unixTime => new Date(unixTime).toLocaleTimeString()}></XAxis>
            <YAxis hide={true}></YAxis>
            <CartesianGrid strokeDasharray="3 3"/>
            <Tooltip content={data => getStatTooltip(data, classes)}
                     labelFormatter={t => new Date(t).toLocaleString()}></Tooltip>
            {Object.entries(getChartValues(rawvalue))
                    .filter(entry => entry[1] !== undefined)
                    .sort((a,b) => sortStats({name:a[0],chartValue:a[1]},{name:b[0],chartValue:b[1]}))
                    .map((entry) => 
                            <Area
                                key={entry[0]}
                                isAnimationActive={statsAnimateChange}
                                type="monotone"
                                fillOpacity={1} 
                                fill={`url(#color${entry[0]})`}
                                stroke={category === "Memory" ? theme.palette.taskManager.memoryColor : theme.palette.taskManager.strokeColor}
                                dataKey={entry[0]}
                                stackId="1"/>)}
        </AreaChart>
    </Box>

    function sortStats(a, b) {
        return a.name === idleField && b.name !== idleField ? 1 : (a.name !== idleField && b.name === idleField ? -1 : a.value-b.value);
    }
});
    const StatsPanel = withStyles(statsStyle)(props => {
    const { classes, siteConfig, open } = props;
    const { statsRefreshRate, statsAnimateChange, maxSamples } = siteConfig;
    const [ statistics, setStatistics] = useState(undefined);
    const [ timer, setTimer ] = useState(undefined);
    const [ lastRefreshDate, setLastRefreshDate] = useState(undefined);
    const [ abortControler, setAbortControler ] = useState(undefined);
    const [ openedCategories, setOpenedCategories ] = useState({
        Package:false,
        CPU: false,
        Memory: false,
        NightDriver: false
    });

    const getStats = (aborter) => fetch(`${httpPrefix !== undefined ? httpPrefix : ""}/getStatistics`,{signal:aborter.signal})
                            .then(resp => resp.json())
                            .then(stats => {
                                setAbortControler(undefined); 
                                return {
                                    Package: {
                                        CHIP: {
                                            stat:{
                                                MODEL: stats.CHIP_MODEL,
                                                CORES: stats.CHIP_CORES,
                                                SPEED: stats.CHIP_SPEED,
                                                PROG_SIZE: stats.PROG_SIZE
                                            },
                                            static: true,
                                            headerFields: ["MODEL"]
                                        },
                                        CODE: {
                                            stat:{
                                                SIZE: stats.CODE_SIZE,
                                                FREE: stats.CODE_FREE,
                                                FLASH_SIZE: stats.FLASH_SIZE
                                            },
                                            static: true,
                                            headerFields: ["SIZE"]
                                        },
                                    },
                                    CPU:{
                                        CPU: {
                                            stat:{
                                                CORE0: stats.CPU_USED_CORE0,
                                                CORE1: stats.CPU_USED_CORE1,
                                                IDLE: ((200.0 - stats.CPU_USED_CORE0 - stats.CPU_USED_CORE1)/200)*100.0,
                                                USED: stats.CPU_USED
                                            },
                                            idleField: "IDLE",
                                            ignored: ["USED"],
                                            headerFields: ["USED"]
                                        }
                                    },
                                    NightDriver: {
                                        FPS:{
                                            stat:{
                                                LED:stats.LED_FPS,
                                                SERIAL:stats.SERIAL_FPS,
                                                AUDIO:stats.AUDIO_FPS
                                            }
                                        },
                                    },
                                    Memory: {
                                        HEAP:{
                                            stat:{
                                                USED:stats.HEAP_SIZE-stats.HEAP_FREE,
                                                FREE:stats.HEAP_FREE,
                                                MIN:stats.HEAP_MIN,
                                                SIZE: stats.HEAP_SIZE
                                            },
                                            idleField: "FREE",
                                            headerFields: ["SIZE","MIN"],
                                            ignored:["SIZE","MIN"]
                                        },
                                        DMA: {
                                            stat:{
                                                USED: stats.DMA_SIZE - stats.DMA_FREE,
                                                FREE: stats.DMA_FREE,
                                                MIN: stats.DMA_MIN,
                                                SIZE: stats.DMA_SIZE
                                            },
                                            idleField: "FREE",
                                            headerFields: ["SIZE","MIN"],
                                            ignored:["SIZE","MIN"]
                                        },
                                        PSRAM: {
                                            stat:{
                                                USED: stats.PSRAM_SIZE - stats.PSRAM_FREE,
                                                FREE: stats.PSRAM_FREE,
                                                MIN: stats.PSRAM_MIN,
                                                SIZE: stats.PSRAM_SIZE
                                            },
                                            idleField: "FREE",
                                            headerFields: ["SIZE","MIN"],
                                            ignored:["SIZE","MIN"]
                                        },
                                    }
                                };
                            });
    useEffect(() => {
        if (abortControler) {
            abortControler.abort();
        }
        const aborter = new AbortController();
        setAbortControler(aborter);

        getStats(aborter).then(setStatistics)
                         .catch(console.error);

        if (timer) {
            clearTimeout(timer);
            setTimer(undefined);
        }

        if (statsRefreshRate.value && open) {
            setTimer(setTimeout(() => setLastRefreshDate(Date.now()),statsRefreshRate.value*1000));
        }

        return () => {
            if (timer) {
                clearTimeout(timer);
            }
            if (abortControler) {
                abortControler.abort();
            }
        }
    },[statsRefreshRate.value, lastRefreshDate, open]);

    if (!statistics) {
        return <Box>Loading...</Box>
    }

    return statistics && 
    <Box className={`${classes.root} ${!open && classes.hidden}`}>
        {Object.entries(statistics).map(category => 
        <Box key={category[0]}>
            <Box className={classes.statCatergoryHeader} key="header">
                <IconButton onClick={()=>setOpenedCategories(prev => {return {...prev,[category[0]]:!openedCategories[category[0]]}})}><Icon>{openedCategories[category[0]] ? "menu" : "expand"}</Icon></IconButton>
                <Typography variant="h5">{category[0]}</Typography>
            </Box>
            <Box className={classes.categoryStats}>
            {Object.entries(category[1])
               .filter(entry=> entry[1].static) 
               .map(entry=>  
                <StaticStatsPanel
                    key={`static-${entry[0]}`}
                    detail={openedCategories[category[0]]}
                    name={entry[0]}
                    stat={entry[1]}/>)}
                <Box className={classes.categoryStats} key="charts">
                    {Object.entries(category[1])
                        .filter(entry=> !entry[1].static) 
                        .map(entry=>  
                            <Box key={`chart-${entry[0]}`} className={classes.chartArea}>
                                {category[1][entry[0]].idleField && <BarStat
                                    key={`Bar-${entry[0]}`}
                                    name={entry[0]}
                                    className={entry[0]}
                                    category={category[0]}
                                    detail={openedCategories[category[0]]}
                                    rawvalue={entry[1].stat}
                                    idleField={ category[1][entry[0]].idleField }
                                    statsAnimateChange={ statsAnimateChange.value }
                                    headerFields={ category[1][entry[0]].headerFields }
                                    ignored={ category[1][entry[0]].ignored || [] } />}
                                <AreaStat
                                    key={`Area-${entry[0]}`}
                                    name={entry[0]}
                                    category={category[0]}
                                    detail={openedCategories[category[0]]}
                                    statsAnimateChange={ statsAnimateChange.value }
                                    rawvalue={entry[1].stat}
                                    maxSamples={ maxSamples.value }
                                    idleField={ category[1][entry[0]].idleField }
                                    headerFields={ category[1][entry[0]].headerFields }
                                    ignored={ category[1][entry[0]].ignored || [] } />
                            </Box>)}
                </Box>
            </Box>
        </Box>)}
    </Box>
});

const BarStat = withStyles(barChartStyle)(props => {
    const { classes, name, rawvalue, ignored, statsAnimateChange , idleField, category, detail } = props;
    const theme = useTheme();

    const getFillColor = ({step, isIdle}) => {
        if (isIdle) {
            return theme.palette.taskManager.idleColor;
        }
        return (theme.palette.taskManager[`${category === "Memory" ? "b" : ""}color${step+1}`]);
    }

    return (
    <Box className={`${classes.summary} ${!detail && classes.minimized}`}>
        <BarChart
            height={detail ? 300 : 80}
            width={detail ? 150 : 100}
            data={[Object.entries(rawvalue)
                .filter(entry=>!["name",...ignored].includes(entry[0]))
                .reduce((ret,entry)=>{ret[entry[0]] = entry[1]; return ret},{name:name})]}
            margin={{
                top: 20,
                right: 30,
                left: 20,
                bottom: 5}}>
            <XAxis hide={true} dataKey="name" />
            <YAxis hide={true} />
            {Object.keys(rawvalue)
                    .filter(field => !ignored.includes(field))
                    .sort(sortStats)
                    .map((field,idx) => <Bar dataKey={field} 
                                                key={field}
                                                stackId="a" 
                                                fill={getFillColor({step: idx, isIdle: field === idleField})} 
                                                isAnimationActive={statsAnimateChange}
                                                type="monotone"
                                                fillOpacity={1}/>)
            }
        </BarChart>
        <Typography variant="summary">{(Object.entries(rawvalue)
                                             .filter(entry => ![idleField,...ignored].includes(entry[0]))
                                             .reduce((ret,stat)=>ret+stat[1],0.0)/
                                       Object.entries(rawvalue)
                                             .filter(entry => !ignored.includes(entry[0]))
                                             .reduce((ret,stat)=>ret+stat[1],0.0)*100).toFixed(0)}%</Typography>
    </Box>)

    function sortStats(a, b) {
        return a === idleField && b !== idleField ? 1 : (a !== idleField && b === idleField ? -1 : a.localeCompare(b));
    }
});
    ReactDOM.createRoot(document.getElementById("root"))
        .render(<MainApp/>);
