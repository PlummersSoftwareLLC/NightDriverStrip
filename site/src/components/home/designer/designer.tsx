import { Box, Typography, ClickAwayListener, TextField, Card, CardHeader, IconButton, Icon, CardContent, LinearProgress, CardActions, List, Button } from "@mui/material";
import { useState, useEffect } from "react";
import { Link } from "react-router-dom";
import { eventManager } from "../../../services/eventManager/eventmanager";
import { Effect } from "./effect/effect";
import { IEffect, IEffects } from '../../../models/config/nightdriver/effects';
import { INightDriverConfiguration } from "../../../models/config/nightdriver/nightdriver";
import { withStyles } from 'tss-react/mui';
import { designerStyle } from "./style";

interface IDesignerPanelProps {
    open: boolean;
    displayMode: string;
    classes?: any;
}

export const DesignerPanel = withStyles((props:IDesignerPanelProps) => {
    const [chipConfig, setChipConfig] = useState(undefined as unknown as INightDriverConfiguration);
    const [service] = useState(eventManager());
    const { classes } = props;

    const [ effects, setEffects ] = useState(undefined as unknown as IEffects);
    const [ nextRefreshDate, setNextRefreshDate] = useState(0);
    const [ editing, setEditing ] = useState(false);
    const [ effectInterval, setEffectInterval ] = useState(0);
    const [ hoverEffect, setHoverEffect ] = useState(undefined as unknown as IEffect);
    const [ displayMode, setDisplayMode ] = useState( props.displayMode );
    
    const [ detailMode ] = useState("list");
    const [ progress, setProgress ] = useState(0);
    

    useEffect(() => {
        if (effects?.millisecondsRemaining) {
            const timeReference = Date.now()+effects.millisecondsRemaining;
            let timeRemaining = timeReference-Date.now();
            const interval = setInterval(()=>{
                const remaining = timeReference-Date.now();
                if (remaining >= 0) {
                    timeRemaining = remaining;
                    setProgress((1-timeRemaining/effects.effectInterval)*100.0);
                } else {
                    service.emit("refreshEffectList")
                }
            },300);
            return ()=>clearInterval(interval);
        }
    },[effects ? effects.millisecondsRemaining:0]);

    useEffect(() => {
        const subs={
            chipConfig:service.subscribe("ChipConfig",cfg=>{setChipConfig(cfg)}),
            effectsSub:service.subscribe("effectList",(effectList:IEffects)=>{
                setEffects(effectList);
                setEffectInterval(effectList.effectInterval);
            }),
        };
        
        return ()=>Object.values(subs).forEach(service.unsubscribe);
    }, [service]);

    useEffect(() => {
        if (props.open) {
            service.emit("refreshEffectList");
            const timer = setTimeout(()=>{
                setNextRefreshDate(Date.now());
            },3000);

            return () => clearTimeout(timer);
        }
    },[props.open,nextRefreshDate]);

    const displayHeader = ()=>{
        return <Box className={classes.effectsHeaderValue}>
            <Typography variant="caption" color="textPrimary">Interval</Typography>:
            <Button color="primary" onClick={() => setEditing(true)}>{effects.effectInterval}</Button>
        </Box>;
    };

    const editingHeader = ()=>{
        return <ClickAwayListener onClickAway={()=>{
                    service.emit("SetChipConfig",{...chipConfig,effectInterval});
                    setEditing(false);
                    setEffects((prev=>{return {...prev,effectInterval}}));
                    setNextRefreshDate(Date.now());
                    }}>
                    <Box className={classes.effectsHeaderValue}>
                        <TextField label="Interval ms"
                            variant="outlined"
                            type="number"
                            defaultValue={effects.effectInterval}
                            onChange={event => setEffectInterval(parseInt(event.target.value))} />
                    </Box>
                </ClickAwayListener>;
    };

    if (!effects && props.open){
        return <Box>Loading....</Box>;
    }

    return effects?.Effects ? 
    <Card variant="outlined" className={`${!open ? classes.hidden : displayMode === "detailed" ? classes.shownAll : classes.shownSome}`}>
        <CardHeader 
                action={<IconButton aria-label="Next" onClick={()=>setDisplayMode(displayMode === "detailed" ? "summary":"detailed")}>
                        <Icon>{displayMode === "detailed" ? "expand_less":"expand_more"}</Icon></IconButton>}
                title={<Box>
                            {effects.Effects.length} effects
                            {displayMode==="detailed"?<IconButton aria-label="Previous" onClick={()=>service.emit("navigate",false)}><Icon>skip_previous</Icon></IconButton>:null}
                            {displayMode==="detailed"?<IconButton aria-label="Next" onClick={()=>service.emit("navigate",true)}><Icon>skip_next</Icon></IconButton>:null}
                            {displayMode==="detailed"?<IconButton aria-label="Refresh Effects" onClick={()=>setNextRefreshDate(Date.now())}><Icon>refresh</Icon></IconButton>:null}
                       </Box>} 
                subheader={editing?editingHeader():displayHeader()} />
        <CardContent sx={{padding:0}}>
            {effectSection()}
            {footer()}
        </CardContent>
        <LinearProgress className={classes.progress} variant="determinate" aria-label={`${Math.floor(progress)}%`} value={progress} />
        <CardActions disableSpacing>
            <IconButton aria-label="Previous" onClick={()=>service.emit("navigate",false)}><Icon>skip_previous</Icon></IconButton>
            <IconButton aria-label="Next" onClick={()=>service.emit("navigate",true)}><Icon>skip_next</Icon></IconButton>
            <IconButton aria-label="Refresh Effects" onClick={()=>setNextRefreshDate(Date.now())}><Icon>refresh</Icon></IconButton>
        </CardActions>
    </Card>:<Box>Loading...</Box>

    function footer() {
        if (displayMode === "summary")
            return (hoverEffect) ? 
                        <Typography variant="caption">{hoverEffect.name}</Typography> : 
                        <Typography variant="caption">{effects.Effects[effects.currentEffect].name}</Typography>;
    }

    function effectSection() {
        switch (detailMode) {
            case "tile":
                return tiles();
            case "list":
                return list();
            default:
                return <Typography>Site error, invalid detail mode:{detailMode}</Typography>
        }
    }

    function list() {
        return <List className={displayMode === "summary" ? classes.summaryEffects : classes.effects}>
                {effects.Effects.map((effect, idx) => <Box key={`effect-${idx}`} onMouseEnter={() => { setHoverEffect(effect); } }
                onMouseLeave={() => { setHoverEffect(undefined as unknown as IEffect); } }>
                <Effect
                    displayMode={displayMode}
                    index={idx}
                    effects={effects}
                    detailMode={detailMode}
                    effectInterval={effects.effectInterval}
                    selected={idx === effects.currentEffect}
                    millisecondsRemaining={effects.millisecondsRemaining} />
            </Box>)}
        </List>
    }

    function tiles() {
        return effects.Effects.map((effect, idx) => <Box key={`effect-${idx}`} onMouseEnter={() => { setHoverEffect(effect); } }
            onMouseLeave={() => { setHoverEffect(undefined as unknown as IEffect); } }>
            <Effect
                displayMode={displayMode}
                detailMode={detailMode}
                key={idx}
                index={idx}
                effectInterval={effects.effectInterval}
                selected={idx === effects.currentEffect}
                millisecondsRemaining={effects.millisecondsRemaining} 
                effects={effects} />
        </Box>);
    }
}, designerStyle);