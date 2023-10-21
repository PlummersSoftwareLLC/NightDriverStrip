import {useState, useEffect} from 'react';
import {IconButton, Icon, Typography, Box, TextField, InputAdornment} from '@mui/material';
import { Dialog, DialogActions, DialogContent, DialogTitle, Button, Checkbox, FormControlLabel } from "@mui/material";
import { statsStyle } from './style';
import httpPrefix from '../../../espaddr';
import StaticStatsPanel from './static/static';
import AreaStat from './areachart/areachart';
import BarStat from './barchart/barchart';
import PropTypes from 'prop-types';

/**
 * 
 * @param {Props} props React Properties.
 * @param {Integer} props.statsRefreshRate The refresh rate to update the chart.
 * @param {Function} props.setStatsRefreshRate Callback to update Stats refresh rate. 
 * @param {Integer} props.maxSamples The max samples to show on a chart.
 * @param {Function} props.setMaxSamples Callback to update max samples. 
 * @param {Boolean} props.animateChart animate the chart if true.
 * @param {Function} props.setStatsRefreshRate Callback to toggle animating the chart. 
 * @param {Boolean} props.open True if the dialog should be open.
 * @param {Function} props.setStatsRefreshRate Callback to close the dialog by setting to false.. 
 * @returns 
 */
const StatsSettingsDialog = ({statsRefreshRate, setStatsRefreshRate, maxSamples, setMaxSamples, animateChart, setAnimateChart, open, setOpen}) => {
    return <Dialog open={open} onClose={ () => setOpen(false)}>
        <DialogTitle>Statistics Configuration</DialogTitle>
        <DialogContent>
            <TextField
                value={statsRefreshRate}
                label="Refresh Rate"
                InputProps={{
                    endAdornment: <InputAdornment>Sec</InputAdornment>,
                    inputProps: { min: 1, max: 60 }
                }}
                type="number"
                id='refresh-rate'
                onChange={e => {
                    const v = e.target.value < 1 ? 1 : e.target.value > 60 ? 60 : e.target.value;
                    setStatsRefreshRate(v);
                }}
                margin="dense"
                fullWidth={true}
                variant="standard"
            />
            <TextField
                value={maxSamples}
                type="number"
                label="Max Samples"
                id='max-samples'
                InputProps={{
                    endAdornment: <InputAdornment>Samples</InputAdornment>,
                    inputProps: { min: 3, max: 250 }
                }}
                onChange={e => {
                    const v = e.target.value < 3 ? 3 : e.target.value > 250 ? 250 : e.target.value;
                    setMaxSamples(v);
                }}
                margin="dense"
                fullWidth={true}
                variant="standard"
            />
            <FormControlLabel label="Animate Chart" control={
                <Checkbox 
                    defaultChecked={animateChart}
                    onChange={(e) => setAnimateChart(e.target.checked)}
                />
            }/>
        </DialogContent>
        <DialogActions>
            <Button onClick={() => setOpen(false)}>Done</Button>
        </DialogActions>
    </Dialog>;
};

const StatsPanel = ({open, addNotification }) => {
    const config = JSON.parse(localStorage.getItem('statsConfig'));
    const [ statistics, setStatistics] = useState(undefined);
    const [ timer, setTimer ] = useState(undefined);
    const [ lastRefreshDate, setLastRefreshDate] = useState(undefined);
    const [ abortControler, setAbortControler ] = useState(undefined);
    // TODO add support for saving these values. Removed from the previous "settings" button in the sidebar
    const [statsRefreshRate, setStatsRefreshRate ] = useState(config && config.statsRefreshRate !== undefined ? config.statsRefreshRate : 3);
    const [maxSamples, setMaxSamples ] = useState(config && config.maxSamples !== undefined ? config.maxSamples : 50);
    const [animateChart, setAnimateChart ] = useState(config && config.animateChart !== undefined ? config.animateChart : false);
    const [ openedCategories, setOpenedCategories ] = useState({
        Package:false,
        CPU: false,
        Memory: false,
        NightDriver: false
    });
    const [settingsOpen, setSettingsOpen] = useState(false);

    // save users state to storage so the page reloads where they left off. 
    useEffect(() => {
        localStorage.setItem('statsConfig', JSON.stringify({
            statsRefreshRate,
            animateChart,
            maxSamples 
        }));
    }, [statsRefreshRate, animateChart, maxSamples]);

    const getStats = (aborter) => fetch(`${httpPrefix !== undefined ? httpPrefix : ""}/statistics`,{signal:aborter.signal})
        .then(resp => resp.json())
        .then(stats => {
            setAbortControler(undefined);
            return {
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
            };
        });

    useEffect(() => {
        if (abortControler) {
            abortControler.abort();
        }

        if (open) {
            const aborter = new AbortController();
            setAbortControler(aborter);

            getStats(aborter)
                .then(setStatistics)
                .catch(err => addNotification("Error","Service","Get Statistics",err));

            if (timer) {
                clearTimeout(timer);
                setTimer(undefined);
            }

            if (statsRefreshRate && open) {
                setTimer(setTimeout(() => setLastRefreshDate(Date.now()),statsRefreshRate*1000));
            }

            return () => {
                timer && clearTimeout(timer);
                abortControler && abortControler.abort();
            };
        }
    },[statsRefreshRate, lastRefreshDate, open]);

    if (!statistics && open) {
        return <Box>Loading...</Box>;
    }

    const hidden = open ? {} : statsStyle.hidden;
    return statistics &&
    <Box sx={{...statsStyle.root, ...hidden}}>
        <IconButton sx={{padding: "0px", height: "100%"}} onClick={() => setSettingsOpen(true)}>
            <Icon> settings</Icon>
        </IconButton>
        {settingsOpen && <StatsSettingsDialog 
            statsRefreshRate={statsRefreshRate} 
            setStatsRefreshRate={setStatsRefreshRate}
            maxSamples={maxSamples}
            setMaxSamples={setMaxSamples}
            animateChart={animateChart}
            setAnimateChart={setAnimateChart}
            open={settingsOpen}
            setOpen={setSettingsOpen}

        />}
        {Object.entries(statistics).map(category => {
            const cat0 = category[0];
            const isOpen = openedCategories[cat0];
            const summarySx = isOpen ? {} : statsStyle.summaryStats; 
            return <Box key={cat0}>
                {isOpen ?
                    <Box sx={statsStyle.statCatergoryHeader} key="header">
                        <Typography variant="h5">{cat0}</Typography>
                        <IconButton onClick={()=>setOpenedCategories(prev => {return {...prev,[cat0]:!isOpen};})}><Icon>minimize</Icon></IconButton>
                    </Box>:
                    <Box>
                        <Typography color="textPrimary">{cat0}</Typography>
                    </Box>}
                <Box sx={statsStyle.categoryStats}>
                    {Object.entries(category[1])
                        .filter(entry=> entry[1].static)
                        .map(entry=>
                        {
                            return <Box
                                key={`entry-${entry[0]}`}
                                sx={{summarySx}}
                                onClick={()=>!openedCategories[category[0]] && setOpenedCategories(prev => {return {...prev,[category[0]]:!openedCategories[category[0]]};})}>
                                <StaticStatsPanel
                                    key={`static-${entry[0]}`}
                                    detail={openedCategories[category[0]]}
                                    name={entry[0]}
                                    stat={entry[1]}/>
                            </Box>;
                        }
                        )}
                    <Box sx={statsStyle.categoryStats} key="charts">
                        {Object.entries(category[1])
                            .filter(entry=> !entry[1].static)
                            .map((entry)=> {
                                return <Box key={`chart-${entry[0]}`}
                                    onClick={()=>!openedCategories[category[0]] && setOpenedCategories(prev => {return {...prev,[category[0]]:!openedCategories[category[0]]};})}
                                    sx={{cursor:"pointer", ...statsStyle.chartArea, ...summarySx}}>
                                    {category[1][entry[0]].idleField && <BarStat
                                        key={`Bar-${entry[0]}`}
                                        name={entry[0]}
                                        className={entry[0]}
                                        category={category[0]}
                                        detail={openedCategories[category[0]]}
                                        rawvalue={entry[1].stat}
                                        idleField={ category[1][entry[0]].idleField }
                                        statsAnimateChange={ animateChart }
                                        headerFields={ category[1][entry[0]].headerFields }
                                        ignored={ category[1][entry[0]].ignored || [] } />}
                                    <AreaStat
                                        key={`Area-${entry[0]}`}
                                        name={entry[0]}
                                        category={category[0]}
                                        detail={openedCategories[category[0]]}
                                        statsAnimateChange={ animateChart }
                                        rawvalue={entry[1].stat}
                                        maxSamples={ maxSamples }
                                        idleField={ category[1][entry[0]].idleField }
                                        headerFields={ category[1][entry[0]].headerFields }
                                        ignored={ category[1][entry[0]].ignored || [] } />
                                </Box>;
                            }
                            )}
                    </Box>
                </Box>
            </Box>;
        }
        )}
    </Box>;
};

StatsPanel.propTypes = {
    open: PropTypes.bool.isRequired,
    addNotification: PropTypes.func.isRequired
};

StatsSettingsDialog.propTypes = {
    statsRefreshRate: PropTypes.number.isRequired, 
    setStatsRefreshRate: PropTypes.func.isRequired, 
    maxSamples: PropTypes.number.isRequired, 
    setMaxSamples: PropTypes.func.isRequired, 
    animateChart: PropTypes.bool.isRequired, 
    setAnimateChart: PropTypes.func.isRequired, 
    open: PropTypes.bool.isRequired, 
    setOpen: PropTypes.func.isRequired
};

export default StatsPanel;