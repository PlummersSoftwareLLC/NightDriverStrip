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

