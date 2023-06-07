import { Typography, ListItem, Paper, Box, Checkbox, CircularProgress, IconButton, Icon, Dialog, DialogTitle, DialogContent, DialogContentText, TextField, DialogActions, Button, Card, CardHeader, Avatar, CardContent } from "@mui/material";
import { useState, useEffect, useMemo } from "react";
import { withStyles } from "tss-react/mui";
import { IEffects } from "../../../../models/config/nightdriver/effects";
import { eventManager } from "../../../../services/eventManager/eventmanager";
import { effectStyle } from "./style";

interface IEffectProps {
    index: number;
    effects: IEffects;
    effectInterval:number;
    millisecondsRemaining: number;
    selected: boolean;
    displayMode: string;
    detailMode: string;
    classes?:any;
}

export const Effect = withStyles(({ effectInterval, millisecondsRemaining, selected, displayMode, detailMode, index, effects, classes }:IEffectProps)=>{
    const [service] = useState(eventManager());

    const [ progress, setProgress ] = useState(0);
    const [ effectSettings, setEffectSettings] = useState();
    const [ dopen, setDOpen ] = useState(false);
    
    const effect = useMemo(()=>effects.Effects[index],[index,...effects.Effects])
    
    useEffect(() => {
        const subs={
            chipConfig:service.subscribe("EffectSettings",cfg=>{setEffectSettings({...cfg})}),
        };
        
        return ()=>Object.values(subs).forEach(service.unsubscribe);
    }, [service]);

    const defaultConfig={
        tile: {
            name: "Effect Image",
            typeName: "url",
            value: "./favicon.ico"
        }
    };

    const fullEffect = useMemo(()=>{
        const getEffectName = (index) => {
            let dups = effects.Effects.map((eff,idx)=>{return{idx,match:eff.name === effects.Effects[index].name}})
                                      .filter(matches=>matches.match);
            if (dups.length > 1) {
                return `${effect.name}_${dups.findIndex(match=>match.idx === index)+1}`
            }
            return effect.name;
        };
    
        const effectName = getEffectName(index);
        return effectSettings && (effectSettings[effectName]) ? {...effect,options:effectSettings[effectName]} : 
                                                                {...effect,options:{...defaultConfig}};
    },[index,...effects.Effects]);

    useEffect(() => {
        if (millisecondsRemaining && selected) {
            const timeReference = Date.now()+millisecondsRemaining;
            let timeRemaining = timeReference-Date.now();
            const interval = setInterval(()=>{
                const remaining = timeReference-Date.now();
                if (remaining >= 0) {
                    timeRemaining = remaining;
                    setProgress((timeRemaining/effectInterval)*100.0);
                }
            },300);
            return ()=>clearInterval(interval);
        }
        if (!selected) {
            setProgress(99);
        }
    },[millisecondsRemaining,selected]);

    if (!fullEffect) {
        return <Typography>Loading...</Typography>
    }

    switch (displayMode) {
        case "summary":
            return summary();
        case "detailed":
            return detailed();
    
        default:
            return <Typography>Site error, invalid display mode {displayMode}</Typography>;
    }

    function detailed() {
        switch (detailMode) {
            case "list":
                return detailedList();
            case "tile":
                return detailedTile();
            default:
                return <Typography>Site error, invalid {detailMode}</Typography>;
        }
    }

    function detailedList() {
        return <ListItem className={`${classes.effectline} ${effect.enabled ? null : classes.disabled}`}>
            <Paper className={classes.effectline}>
                <Box className={`${selected ? classes.activelightbar : classes.lightbar}`}></Box>
                {getEffectOptionDialog()}
                {selected?<Box className={classes.line}>
                    <Box className={classes.effectName}>
                        <Checkbox checked={fullEffect.enabled} onChange={()=>service.emit("toggleEffect", effect)} />
                        <img style={{height: 60}} src={fullEffect.options.tile.value}/>
                    </Box>
                    <Typography>{fullEffect.name}</Typography>
                    <Box>
                        <CircularProgress aria-label={`${Math.floor(progress)}%`} variant="determinate" value={progress} color="primary" />
                    </Box>
                </Box>:<Box className={classes.line}>
                    <Box  className={classes.effectDetail}>
                        <Checkbox checked={fullEffect.enabled} onChange={()=>service.emit("toggleEffect", effect)} />
                        <Box className={classes.effectName}>
                            <img style={{height: 60}} src={fullEffect.options.tile.value}/>
                            <Typography>{fullEffect.name}</Typography>
                            <IconButton onClick={()=>setDOpen(true)}><Icon>settings</Icon></IconButton>
                        </Box>
                    </Box>
                    <Box className={classes.listButtons}>
                        {!selected && fullEffect.enabled && <IconButton aria-label="Select Effect" color="secondary" onClick={() => service.emit("navigateTo", index)}><Icon>play_circle_outline</Icon></IconButton>}
                        {selected && <CircularProgress aria-label={`${Math.floor(progress)}`} variant="determinate" value={progress} color="primary" />}
                    </Box>
                </Box>}
            </Paper>
        </ListItem>
    }

    function getEffectOptionDialog() {
        const [options, setOptions] = useState(fullEffect.options);
        const save = () => {
            service.emit("setEffectSettings",{index,options});
            setDOpen(false);
        };
        return (
              <Dialog open={dopen} onClose={()=>setDOpen(false)}>
                <DialogTitle>{fullEffect.name} Options</DialogTitle>
                <DialogContent>
                  <DialogContentText>
                    Display Options 
                  </DialogContentText>
                  {Object.entries(fullEffect.options).map(entry=>
                    <TextField
                        autoFocus
                        margin="dense"
                        id={entry[0]}
                        key={entry[0]}
                        label={entry[1].name}
                        type={entry[1].typeName}
                        defaultValue={entry[1].value}
                        fullWidth
                        variant="standard"
                        onChange={evt=>setOptions(prev=>{return{...prev,[entry[0]]:{...entry[1],value:evt.target.value}}})}
                    />)}
                </DialogContent>
                <DialogActions>
                  <Button onClick={()=>setDOpen(false)}>Cancel</Button>
                  <Button onClick={save}>Save</Button>
                </DialogActions>
              </Dialog>
          );      
    }

    function detailedTile() {
        return <Card variant="outlined">
            <CardHeader
                avatar={<Avatar aria-label={fullEffect.name}>
                    {fullEffect.name[0]}
                </Avatar>}
                title={fullEffect.name}
                subheader={fullEffect.enabled ? (selected ? "Active" : "") : "Disabled"}
                className={classes.cardheader}/>
            <CardContent className={classes.cardcontent}>
                {selected &&
                    <div className={classes.circularProgress}>
                        <CircularProgress aria-label={`${Math.floor(progress)}$`} variant="determinate" value={progress} color="primary" />
                        <Typography className={classes.circularProgressText} color="textSecondary" variant="caption">{Math.floor(progress)}</Typography>
                    </div>}
                {!selected && <IconButton aria-label="Toggle Effect" color="secondary" onClick={() => service.emit("toggleEffect", effect)}>{<Icon>{fullEffect.enabled ? "block" : "add_alarm"}</Icon>}</IconButton>}
                {!selected && fullEffect.enabled && <IconButton aria-label="Select Effect" color="secondary" onClick={() => service.emit("navigateTo", index)}><Icon>play_circle_outline</Icon></IconButton>}
            </CardContent>
        </Card>;
    }

    function summary() {
        return <div className={`${classes.dot} ${selected ? classes.selected : classes.waiting}`}
                    onClick={() => service.emit("navigateTo", index)} />;
    }
}, effectStyle);