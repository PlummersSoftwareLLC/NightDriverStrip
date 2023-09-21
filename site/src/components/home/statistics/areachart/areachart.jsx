import { useState, useMemo } from "react";
import { AreaChart, Area, CartesianGrid, XAxis, YAxis, Tooltip } from 'recharts'
import {useTheme, Box, Typography, List, ListItem } from "@mui/material";
import areaChartStyle from "./style";

const AreaStat = props => {
    const {name, rawvalue, ignored, statsAnimateChange, maxSamples, headerFields , idleField, category, detail } = props;
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
    };

    const getStatTooltip = (data, classes) => {
        return (
            <Box sx={classes.tooltipContent}>
                <Box sx={classes.tooltipHeader}>{data.labelFormatter(data.label)}</Box>
                <List sx={classes.threads}>
                    {data.payload
                        .sort((a,b) => sortStats(b,a))
                        .map(stat => 
                            <ListItem key={stat.name} sx={classes.thread}>
                                <Box sx={classes.threadName} style={{color:stat.color}}>{stat.name}</Box>
                                <Box sx={classes.threadValue}>{getValue(stat.value)}
                                    <Box sx={classes.threadSummary}>
                                ({(stat.value/data.payload.reduce((ret,stat) => ret + stat.value,0)*100).toFixed(2)}%)
                                    </Box>
                                </Box>
                            </ListItem>)
                    }
                </List>
            </Box>)
    };

    const sortStats = (a, b) => {
        return a.name === idleField && b.name !== idleField ? 1 : (a.name !== idleField && b.name === idleField ? -1 : a.value-b.value);
    };

    return <Box sx={areaChartStyle.root}>
        {detail && <Box sx={areaChartStyle.header}>
            <Typography sx={areaChartStyle.headerLine} color="textPrimary" variant="subtitle1">{name} {headerFields && Object.values(headerFields).map(headerField=>
                <Typography key={headerField} sx={areaChartStyle.headerField} color="textPrimary" variant="subtitle2">{headerField}: 
                    <Typography color="textSecondary" variant="subtitle2">{rawvalue[headerField]}</Typography>
                </Typography>)}
            </Typography>
            <List sx={areaChartStyle.stats}>
                {Object.entries(rawvalue)
                    .filter(entry=>!ignored.includes(entry[0]))
                    .map(entry=>
                        <ListItem sx={areaChartStyle.stats} key={entry[0]}>
                            <Typography color="textPrimary" variant="little">{entry[0]}</Typography>:
                            <Typography color="textSecondary" variant="little" >{getValue(entry[1])}</Typography>
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
            {<Tooltip content={data => getStatTooltip(data, areaChartStyle)}
                labelFormatter={t => new Date(t).toLocaleString()}></Tooltip>}
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
};
    
export default AreaStat;