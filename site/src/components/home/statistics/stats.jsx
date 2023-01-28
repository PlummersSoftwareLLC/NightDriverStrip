var statsCallbacks = {};

const StatsPanel = withStyles(statsStyle)(props => {
    const { classes, siteConfig } = props;
    const { statsRefreshRate, statsAnimateChange, maxSamples } = siteConfig;
    const [ statistics, setStatistics] = React.useState(undefined);
    const [ timer, setTimer ] = React.useState(undefined);
    const [ refreshRate, setRefreshRate ] = React.useState(statsRefreshRate);

    const registerStatCallback = (name,callback) => statsCallbacks = {...statsCallbacks,[name]:callback};
    const getStats = () => {
        return fetch(`${httpPrefix !== undefined ? httpPrefix : ""}/getStatistics`)
            .then(resp => resp.json())
            .then(stats => {return {
                FPS:{
                    LED:stats.LED_FPS,
                    SERIAL:stats.SERIAL_FPS,
                    AUDIO:stats.AUDIO_FPS
                },
                HEAP:{
                    USED:stats.HEAP_SIZE-stats.HEAP_FREE,
                    FREE:stats.HEAP_FREE,
                    MIN:stats.HEAP_MIN
                },
                DMA: {
                    USED: stats.DMA_SIZE - stats.DMA_FREE,
                    FREE: stats.DMA_FREE,
                    MIN: stats.DMA_MIN
                },
                PSRAM: {
                    USED: stats.PSRAM_SIZE - stats.PSRAM_FREE,
                    FREE: stats.PSRAM_FREE,
                    MIN: stats.PSRAM_MIN
                },
                CHIP: {
                    MODEL: stats.CHIP_MODEL,
                    CORES: stats.CHIP_CORES,
                    SPEED: stats.CHIP_SPEED,
                    PROG_SIZE: stats.PROG_SIZE
                },
                CODE: {
                    SIZE: stats.CODE_SIZE,
                    FREE: stats.CODE_FREE,
                    FLASH_SIZE: stats.FLASH_SIZE,
                },
                CPU: {
                    CORE0: stats.CPU_USED_CORE0,
                    CORE1: stats.CPU_USED_CORE1,
                    IDLE: ((200.0 - stats.CPU_USED_CORE0 - stats.CPU_USED_CORE1)/200)*100.0,
                }
            }});
    }    

    if (statistics && (!timer || (refreshRate.value !== statsRefreshRate.value))) {
        if (timer) {
            clearInterval(timer);
        }
        setRefreshRate(statsRefreshRate);

        setTimer(setInterval(() => getStats().then(stats => Object.entries(statistics).forEach(stat => statsCallbacks[stat[0]](stats[stat[0]]))),statsRefreshRate.value*1000));
    }

    !statistics && getStats().then(stats => {
        const updatedStats = {
            FPS:stats.FPS, 
            HEAP:stats.HEAP,
            DMA: stats.DMA,
            PSRAM: stats.PSRAM,
            CPU: stats.CPU
        };
        setStatistics(updatedStats);
        setTimer(setInterval(() => getStats().then(stats => Object.entries(updatedStats).forEach(stat => statsCallbacks[stat[0]](stats[stat[0]]))),statsRefreshRate.value*1000));
    });

    const getIgnored = (name) => {
        if ((name === "DMA") ||
            (name === "PSRAM") || 
            (name === "HEAP")) {
            return ["MIN"];
        }
        return [];
    };

    const getHeaderField = (name) => {
        if ((name === "DMA") ||
            (name === "DPSRAM") || 
            (name === "HEAP")) {
            return "MIN";
        }
    };

    return statistics && <Box className={classes.root}>
        {Object.entries(statistics)
            .map(entry=><AreaStat
                    key={entry[0]}
                    name={entry[0]}
                    registerStatCallback={registerStatCallback}
                    rawvalue={entry[1]}
                    statsAnimateChange={ statsAnimateChange.value }
                    maxSamples={ maxSamples.value }
                    headerField={getHeaderField(entry[0])}
                    ignored={getIgnored(entry[0])}/>)}
    </Box>
});

