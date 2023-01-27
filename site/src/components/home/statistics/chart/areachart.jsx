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

const AreaStat = withStyles(chartStyle)(props => {
    const { classes, name, rawvalue, ignored, statsRefreshRate, statsAnimateChange, maxSamples } = props;
    const value = Object.entries(rawvalue)
                        .filter(entry=>!ignored.includes(entry[0]))
                        .reduce((ret,entry)=>{ret[entry[0]] = entry[1]; return ret},{});
    const [lastStates, setLastStates] = React.useState([Object.entries(value)
                                             .reduce((ret,stat)=>{ret[stat[0]]=stat[1]; return ret},{ts: new Date().getTime()})]);
    const [ timer, setTimer ] = React.useState(undefined);
    const [ refreshRate, setRefreshRate ] = React.useState(statsRefreshRate);

    if (!timer || (refreshRate !== statsRefreshRate)) {
        if (timer) {
            clearInterval(timer);
        }
        setRefreshRate(statsRefreshRate);

        setTimer(setInterval(() => setLastStates((prevState)=>[...prevState,Object.entries(value)
                    .reduce((ret,stat)=>{ret[stat[0]]=stat[1]*(Math.random()); return ret},{ts: new Date().getTime()})]
                    .filter((_val,idx,arr) => arr.length >= maxSamples ? idx > arr.length - maxSamples : true)),statsRefreshRate*1000));
    }

    return <Box className={classes.root}>
        <Box className={classes.header}>
            <Typography variant="h7">{name}</Typography>
            <List className={classes.stats}>
                {Object.entries(rawvalue).map(entry=>
                    <ListItem className={classes.stats} key={entry[0]}>
                        <Typography variant="littleHeader">{entry[0]}</Typography>:
                        <Typography variant="littleValue" >{entry[1]}</Typography>
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
                <Tooltip className="tooltip"
                         content={"tips"}
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
    