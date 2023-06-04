const StatsPanel = withStyles(statsStyle)(props => {
    const [siteConfig, setSiteConfig] = useState({});
    const [service] = useState(eventManager());

    const { classes, open, smallScreen } = props;
    const [ statistics, setStatistics] = useState(undefined);
    const [ timer, setTimer ] = useState(undefined);
    const [ lastRefreshDate, setLastRefreshDate] = useState(undefined);
    const [ openedCategories, setOpenedCategories ] = useState({
        Package:false,
        CPU: false,
        Memory: false,
        NightDriver: false
    });
    useEffect(() => {
        const subs={
            siteConfig:service.subscribe("SiteConfig",setSiteConfig),
            stats:service.subscribe("statistics",stats=>{setStatistics(toGraphData(stats))})
        };
        function toGraphData(stats) {
            return {
                CPU: {
                    CPU: {
                        stat: {
                            CORE0: stats.CPU_USED_CORE0,
                            CORE1: stats.CPU_USED_CORE1,
                            IDLE: ((200.0 - stats.CPU_USED_CORE0 - stats.CPU_USED_CORE1) / 200) * 100.0,
                            USED: stats.CPU_USED
                        },
                        idleField: "IDLE",
                        ignored: ["USED"],
                        headerFields: ["USED"]
                    }
                },
                Memory: {
                    HEAP: {
                        stat: {
                            USED: stats.HEAP_SIZE - stats.HEAP_FREE,
                            FREE: stats.HEAP_FREE,
                            MIN: stats.HEAP_MIN,
                            SIZE: stats.HEAP_SIZE
                        },
                        idleField: "FREE",
                        headerFields: ["SIZE", "MIN"],
                        ignored: ["SIZE", "MIN"]
                    },
                    DMA: {
                        stat: {
                            USED: stats.DMA_SIZE - stats.DMA_FREE,
                            FREE: stats.DMA_FREE,
                            MIN: stats.DMA_MIN,
                            SIZE: stats.DMA_SIZE
                        },
                        idleField: "FREE",
                        headerFields: ["SIZE", "MIN"],
                        ignored: ["SIZE", "MIN"]
                    },
                    PSRAM: {
                        stat: {
                            USED: stats.PSRAM_SIZE - stats.PSRAM_FREE,
                            FREE: stats.PSRAM_FREE,
                            MIN: stats.PSRAM_MIN,
                            SIZE: stats.PSRAM_SIZE
                        },
                        idleField: "FREE",
                        headerFields: ["SIZE", "MIN"],
                        ignored: ["SIZE", "MIN"]
                    },
                },
                NightDriver: {
                    FPS: {
                        stat: {
                            LED: stats.LED_FPS,
                            SERIAL: stats.SERIAL_FPS,
                            AUDIO: stats.AUDIO_FPS
                        }
                    },
                },
                Package: {
                    CHIP: {
                        stat: {
                            MODEL: stats.CHIP_MODEL,
                            CORES: stats.CHIP_CORES,
                            SPEED: stats.CHIP_SPEED,
                            PROG_SIZE: stats.PROG_SIZE
                        },
                        static: true,
                        headerFields: ["MODEL"]
                    },
                    CODE: {
                        stat: {
                            SIZE: stats.CODE_SIZE,
                            FREE: stats.CODE_FREE,
                            FLASH_SIZE: stats.FLASH_SIZE
                        },
                        static: true,
                        headerFields: ["SIZE"]
                    },
                },
            };
        }

        return ()=>Object.values(subs).forEach(service.unsubscribe);
    }, [service]);

    useEffect(() => {
        if (open) {
            service.emit("refreshStatistics");

            if (siteConfig && (siteConfig.statsRefreshRate.value >= 0) && open) {
                setTimer(setTimeout(() => setLastRefreshDate(Date.now()),siteConfig.statsRefreshRate.value*1000));
            }

            return () => {timer && clearTimeout(timer)};
        }
    },[...siteConfig, lastRefreshDate, open]);

    if ((!statistics || !siteConfig) && open) {
        return <Box>Loading...</Box>
    }

    return statistics && siteConfig && 
    <Card variant="outlined" className={`${!open && classes.hidden}`}>
        <CardContent className={Object.values(openedCategories).some(category => category)?classes.contentDetails:(smallScreen?classes.contentSummarySmall:classes.contentSummaryBig)}>
            {Object.entries(statistics).map(category =>
                <Accordion key={category[0]}
                           expanded={openedCategories[category[0]]} 
                           onChange={()=>setOpenedCategories(prev => {return {...prev,[category[0]]:!openedCategories[category[0]]}})}>
                    <AccordionSummary
                        expandIcon={<Icon>expand_more</Icon>}>
                        <Box className={classes.headerStats}>
                            <Typography sx={{ width: '50%', flexShrink: 0 }}>
                                {category[0]}
                            </Typography>
                            {!openedCategories[category[0]] && <Box className={classes.smallgraphs}>
                                {Object.entries(category[1]).filter(entry=>!entry[1].static && entry[1].idleField).map(entry => <BarStat
                                            key={`Bar-${entry[0]}`}
                                            name={entry[0]}
                                            className={entry[0]}
                                            category={category[0]}
                                            detail={openedCategories[category[0]]}
                                            rawvalue={entry[1].stat}
                                            idleField={ category[1][entry[0]].idleField }
                                            statsAnimateChange={ siteConfig.statsAnimateChange.value }
                                            headerFields={ category[1][entry[0]].headerFields }
                                            ignored={ category[1][entry[0]].ignored || [] } />)}
                            </Box>}
                        </Box>
                    </AccordionSummary>
                    <AccordionDetails>
                        <Box className={classes.detailStats}>
                            {Object.entries(category[1]).map((entry)=> entry[1].static ?
                                <Box key={`entry-${entry[0]}`}
                                    className={!openedCategories[category[0]] && classes.summaryStats }>
                                    <StaticStatsPanel
                                        key={`static-${entry[0]}`}
                                        detail={openedCategories[category[0]]}
                                        name={entry[0]}
                                        stat={entry[1]}/>
                            </Box>:
                            <Box key={`chart-${entry[0]}`}
                                sx={{cursor:"pointer"}}
                                className={`${classes.chartArea} ${!openedCategories[category[0]] && classes.summaryStats}`}>
                                    {category[1][entry[0]].idleField && 
                                    <BarStat
                                        key={`Bar-${entry[0]}`}
                                        name={entry[0]}
                                        className={entry[0]}
                                        category={category[0]}
                                        detail={openedCategories[category[0]]}
                                        rawvalue={entry[1].stat}
                                        idleField={ category[1][entry[0]].idleField }
                                        statsAnimateChange={ siteConfig.statsAnimateChange.value }
                                        headerFields={ category[1][entry[0]].headerFields }
                                        ignored={ category[1][entry[0]].ignored || [] } />}
                                    <AreaStat
                                        key={`Area-${entry[0]}`}
                                        name={entry[0]}
                                        category={category[0]}
                                        detail={openedCategories[category[0]]}
                                        statsAnimateChange={ siteConfig.statsAnimateChange.value }
                                        rawvalue={entry[1].stat}
                                        maxSamples={ siteConfig.maxSamples.value }
                                        idleField={ category[1][entry[0]].idleField }
                                        headerFields={ category[1][entry[0]].headerFields }
                                        ignored={ category[1][entry[0]].ignored || [] } />
                            </Box>)}
                        </Box>
                    </AccordionDetails>
                </Accordion>)}
        </CardContent>
    </Card>
});

