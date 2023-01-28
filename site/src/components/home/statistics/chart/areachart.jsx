/**
 * @param numOfSteps: Total number steps to get color, means total colors
 * @param step: The step number, means the order of the color
 */
const rainbow = (numOfSteps, step) => {
    // This function generates vibrant, "evenly spaced" colours (i.e. no clustering). This is ideal for creating easily distinguishable vibrant markers in Google Maps and other apps.
    // Adam Cole, 2011-Sept-14
    // HSV to RBG adapted from: http://mjijackson.com/2008/02/rgb-to-hsl-and-rgb-to-hsv-color-model-conversion-algorithms-in-javascript
    
    let r, g, b;
    let h = step / numOfSteps;
    let i = ~~(h * 6);
    let f = h * 6 - i;
    let q = 1 - f;
    // eslint-disable-next-line default-case
    switch(i % 6){
        case 0: r = 1; g = f; b = 0; break;
        case 1: r = q; g = 1; b = 0; break;
        case 2: r = 0; g = 1; b = f; break;
        case 3: r = 0; g = q; b = 1; break;
        case 4: r = f; g = 0; b = 1; break;
        case 5: r = 1; g = 0; b = q; break;
    }
    let c = "#" + ("00" + (~ ~(r * 255)).toString(16)).slice(-2) + ("00" + (~ ~(g * 255)).toString(16)).slice(-2) + ("00" + (~ ~(b * 255)).toString(16)).slice(-2);
    return (c);
}

const getStatTooltip = (data, classes) => {
    return <div className={classes.tooltipContent}>
        <div className={classes.tooltipHeader}>{data.labelFormatter(data.label)}</div>
        <ul className={classes.threads}>
            {data.payload
                 .sort((a,b) => a.value === b.value ? 0 : (a.value < b.value || -1))
                 .map(stat => <div key={stat.name} className={classes.thread}>
                <div className={classes.threadName} style={{color:stat.fill === "black" ? "lightgreen" : stat.fill}}>{stat.name}</div>
                <div className={classes.threadValue}>{stat.value.toFixed(2)}<div className={classes.threadSummary}>
                    ({(stat.value/data.payload.reduce((ret,stat) => ret + stat.value,0)*100).toFixed(2)}%)
                    </div></div>
            </div>)}
        </ul>
    </div>
}

const AreaStat = withStyles(chartStyle)(props => {
    const { classes, name, rawvalue, ignored, statsAnimateChange, maxSamples, registerStatCallback, headerField } = props;
    const [value, setValue] = React.useState(Object.entries(rawvalue)
                        .filter(entry=>!ignored.includes(entry[0]))
                        .reduce((ret,entry)=>{ret[entry[0]] = entry[1]; return ret},{}));
    const [lastStates, setLastStates] = React.useState([Object.entries(value)
                                             .reduce((ret,stat)=>{ret[stat[0]]=stat[1]; return ret},{ts: new Date().getTime()})]);
    
    registerStatCallback && registerStatCallback(name, value => {
        setValue(value);
        setLastStates([...lastStates,Object.entries(value)
            .filter(entry=>!ignored.includes(entry[0]))
            .reduce((ret,stat)=>{ret[stat[0]]=stat[1]; return ret},{ts: new Date().getTime()})]
            .filter((_val,idx,arr) => arr.length >= maxSamples ? idx > arr.length - maxSamples : true));
    });

    return <Box className={classes.root}>
        <Box className={classes.header}>
            <Typography variant="h7">{name} {headerField && value[headerField] !== undefined && (`${headerField}: ${value[headerField]}`)}</Typography>
            <List className={classes.stats}>
                {Object.entries(value)
                        .filter(entry=>!ignored.includes(entry[0]))
                        .map(entry=>
                    <ListItem className={classes.stats} key={entry[0]}>
                        <Typography variant="littleHeader">{entry[0]}</Typography>:
                        <Typography variant="littleValue" >{entry[1] !== undefined && !Number.isInteger(entry[1]) ? entry[1].toFixed(2) : entry[1]}</Typography>
                    </ListItem>)}
            </List>
        </Box>
        <AreaChart 
                data={lastStates}
                height={300}
                width={500}
                className="chart"
                stackOffset="expand">
                <CartesianGrid strokeDasharray="3 3"></CartesianGrid>
                <XAxis dataKey="ts"
                       name='Time'
                       color='black'
                       tickFormatter={unixTime => new Date(unixTime).toLocaleTimeString()}></XAxis>
                <YAxis hide={true}></YAxis>
                <Tooltip content={data => getStatTooltip(data, classes)}
                         labelFormatter={t => new Date(t).toLocaleString()}></Tooltip>
                {Object.keys(value).map((line,idx,arr) => <Area
                                    key={line}
                                    isAnimationActive={statsAnimateChange}
                                    type="monotone"
                                    fill={rainbow(arr.length,idx)}
                                    dataKey={line}
                                    stackId="1"></Area>)}
        </AreaChart>
    </Box>
});
    