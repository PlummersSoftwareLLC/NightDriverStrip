const StatsPanel = withStyles(statsStyle)(props => {
    const { classes, siteConfig, open } = props;
    const { statsRefreshRate, statsAnimateChange, maxSamples } = siteConfig;
    const [ statistics, setStatistics] = React.useState(undefined);
    const [ timer, setTimer ] = React.useState(undefined);
    const [ statsCallbacks, setStatsCallbacks ] = React.useState({});
    const [ lastRefreshDate, setLastRefreshDate] = React.useState(undefined);
    const [ abortControler, setAbortControler ] = React.useState(undefined);
    const [ openedCategories, setOpenedCategories ] = React.useState({
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
                                            ignored:["MODEL"],
                                            headerField: "MODEL"
                                        },
                                        CODE: {
                                            stat:{
                                                SIZE: stats.CODE_SIZE,
                                                FREE: stats.CODE_FREE,
                                                FLASH_SIZE: stats.FLASH_SIZE
                                            },
                                            static: true
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
                                            ignored: "USED",
                                            headerField: "USED"
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
                                            headerField: ["SIZE"],
                                            ignored:["SIZE"]
                                        },
                                        DMA: {
                                            stat:{
                                                USED: stats.DMA_SIZE - stats.DMA_FREE,
                                                FREE: stats.DMA_FREE,
                                                MIN: stats.DMA_MIN,
                                                SIZE: stats.DMA_SIZE
                                            },
                                            idleField: "FREE",
                                            headerField: ["SIZE"],
                                            ignored:["SIZE"]
                                        },
                                        PSRAM: {
                                            stat:{
                                                USED: stats.PSRAM_SIZE - stats.PSRAM_FREE,
                                                FREE: stats.PSRAM_FREE,
                                                MIN: stats.PSRAM_MIN,
                                                SIZE: stats.PSRAM_SIZE
                                            },
                                            idleField: "FREE",
                                            headerField: ["SIZE"],
                                            ignored:["SIZE"]
                                        },
                                    }
                                };
                            });

    React.useEffect(() => {
        if (abortControler) {
            abortControler.abort();
        }
        const aborter = new AbortController();
        setAbortControler(aborter);

        getStats(aborter).then(stats => setStatistics(_prevStats => {
            Object.entries(stats)
                  .forEach(category => Object.entries(category[1])
                    .filter(stat => statsCallbacks[stat[0]])
                    .forEach(stat => statsCallbacks[stat[0]](stat[1].stat)));
            return stats;
        })).catch(console.error);

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
            <Box className={classes.statCatergoryHeader}>
                <IconButton onClick={()=>setOpenedCategories(prev => {return {...prev,[category[0]]:!openedCategories[category[0]]}})}><Icon>{openedCategories[category[0]] ? "menu" : "expand"}</Icon></IconButton>
                <Typography variant="h5">{category[0]}</Typography>
            </Box>
            {Object.entries(category[1])
               .filter(entry=> entry[1].static) 
               .map(entry=>  
                <StaticStatsPanel
                    key={entry[0]}
                    detail={openedCategories[category[0]]}
                    name={entry[0]}
                    stat={entry[1]}/>)}
            <Box className={classes.categoryStats}>
                {Object.entries(category[1])
                    .filter(entry=> !entry[1].static) 
                    .map(entry=>  
                        <AreaStat
                            key={entry[0]}
                            name={entry[0]}
                            category={category[0]}
                            detail={openedCategories[category[0]]}
                            registerStatCallback={(name,callback) => name !== undefined && callback !== undefined && setStatsCallbacks(prevStats => {return {...prevStats||{},[name]:callback}})}
                            rawvalue={entry[1].stat}
                            statsAnimateChange={ statsAnimateChange.value }
                            maxSamples={ maxSamples.value }
                            idleField={ category[1][entry[0]].idleField }
                            headerField={ category[1][entry[0]].headerField }
                            ignored={ category[1][entry[0]].ignored || [] } />)}
            </Box>
        </Box>)}
    </Box>
});

