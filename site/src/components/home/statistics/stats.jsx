const StatsPanel = withStyles(statsStyle)(props => {
const { classes, siteConfig } = props;
const { statsRefreshRate, statsAnimateChange, maxSamples } = siteConfig;
const [statistics, setStatistics] = React.useState({
    FPS:{
        LED_FPS:25,
        SERIAL_FPS: 15,
        AUDIO_FPS: 23
    },
    HEAP:{
        HEAP_SIZE:9000,
        HEAP_FREE:4096,
        HEAP_MIN:2048
    },
    DMA: {
        DMA_SIZE: 1034,
        DMA_FREE: 1034,
        DMA_MIN: 1034
    },
    PSRAM: {
        PSRAM_SIZE: 2354,
        PSRAM_FREE: 2354,
        PSRAM_MIN: 2354
    },
    CHIP: {
        CHIP_MODEL: "ESP32-S2FH4",
        CHIP_CORES: "2",
        CHIP_SPEED: "80",
        PROG_SIZE: "2048",
    },
    CODE: {
        CODE_SIZE: 4096,
        CODE_FREE: 4096,
        FLASH_SIZE: 4096,
    },
    CPU: {
        CPU_USED: .8,
        CPU_USED_CORE0: .35,
        CPU_USED_CORE1: .45
    }
});

return <Box className={classes.root}>
    {Object.entries(statistics)
        //    .filter((_val,idx)=>idx===0)
           .map(entry=><AreaStat
                key={entry[0]}
                name={entry[0]}
                rawvalue={entry[1]}
                statsRefreshRate={statsRefreshRate.value}
                statsAnimateChange={ statsAnimateChange.value }
                maxSamples={ maxSamples.value }
                ignored={[Object.keys(entry[1])[0]]}/>)}
</Box>});

