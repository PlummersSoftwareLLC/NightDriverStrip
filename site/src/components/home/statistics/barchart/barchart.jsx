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
        <Typography className={classes.pct} variant="summary">{(Object.entries(rawvalue)
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
    