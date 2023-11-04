import {useState, useEffect, useContext} from 'react';
import {IconButton, Icon, Card, CardHeader, CardContent, Avatar, CardActions, Box, Checkbox} from '@mui/material';
import {LinearProgress, CircularProgress, Button, useTheme} from '@mui/material';
import effectStyle from './style';
import PropTypes from 'prop-types';
import ConfigDialog from '../../config/configDialog';
import { EffectsContext } from '../../../../context/effectsContext';
import { height } from '@mui/system';

const Effect = props => {
    const {activeInterval,remainingInterval, pinnedEffect, currentEffect} = useContext(EffectsContext);
    const { effect, effectIndex, effectEnable, navigateTo, requestRunning, gridLayout, onDragStart, onDragOver} = props;
    const [ progress, setProgress ] = useState(0);
    const [open, setOpen] = useState(false);
    const selected = Number(effectIndex) === currentEffect;

    const theme = useTheme();
    const classes = effectStyle(theme);
    useEffect(() => {
        if (selected) {
            if (remainingInterval) {
                const timeReference = Date.now()+remainingInterval;
                var timeRemaining = timeReference-Date.now();
                const interval = setInterval(()=>{
                    const remaining = timeReference-Date.now();
                    if (remaining >= 0) {
                        timeRemaining = remaining;
                        setProgress((timeRemaining/activeInterval)*100.0);
                    }
                },300);
                return ()=>clearInterval(interval);
            }
        } else {
            setProgress(0);
        }
    },[remainingInterval,selected, activeInterval]);

    return <Card variant="outlined" sx={gridLayout ? classes.gridCard : classes.listCard} draggable 
        onDragStart={(event) => onDragStart(event, effectIndex)} 
        onDragOver={(event) => onDragOver(event, effectIndex)}
    >
        { gridLayout ? <>
            <CardHeader
                avatar={
                    <Avatar aria-label={effect.name}>
                        {effect.name[0]}
                    </Avatar>
                }
                title={effect.name}
                subheader={effect.enabled?(selected?"Active":"Waiting") : "Disabled"}
                sx={classes.cardheader}
            /> 
            <CardContent>
                {selected && (pinnedEffect ? <Box sx={{textAlign: 'center'}}><Icon>all_inclusive</Icon></Box> : <LinearProgress disabled={requestRunning} variant="determinate" sx={{transition: 'none'}} value={progress} />)}
                {!selected && <Button disabled={requestRunning} onClick={()=>effectEnable(effectIndex,!effect.enabled)} variant="outlined" startIcon={<Icon >{effect.enabled?"stop":"circle"}</Icon>}>{effect.enabled?"Disable":"Enable"}</Button>}
                {!selected && effect.enabled && <Button disabled={requestRunning} onClick={()=>navigateTo(effectIndex)} variant="outlined" startIcon={<Icon >start</Icon>}>Trigger</Button>}
            </CardContent>
            <CardActions disableSpacing>
                <IconButton
                    onClick={()=>setOpen(true)}
                    aria-label="show more">
                    <Icon>settings</Icon>
                </IconButton>
            </CardActions>
        </>
            : <Box onClick={()=>{setOpen(true);}} sx={{display: "flex", height: "100%"}}>
                <Box sx={{...classes.listColumn, textAlign: "left"}} flexDirection={"row"} display={"flex"}>
                    <Icon sx={{marginTop: '2%'}}>drag_handle</Icon>
                    <Checkbox checked={effect.enabled} disabled={selected} onClick={(e)=>{e.stopPropagation(); effectEnable(effectIndex,!effect.enabled);}} sx={classes.short}/>
                    <CardHeader sx={classes.short}
                        avatar={
                            <Avatar aria-label={effect.name} sx={{width: "30px", height: "30px"}}>
                                {effect.name[0]}
                            </Avatar>
                        }
                    /> 
                </Box>
                <Box sx={{...classes.listColumn, textAlign: "center"}}>
                    <Box>
                        {effect.name}
                    </Box>
                </Box>
                <Box sx={{...classes.listColumn, textAlign: "-moz-right"}}>
                    {selected && <Box width="64px" paddingLeft={"8px"} paddingRight={"8px"} textAlign={"center"} height={"100%"}>
                        {pinnedEffect ? <Icon sx={{marginTop: '25%'}}>all_inclusive</Icon>
                            : <CircularProgress variant="determinate" sx={{marginTop: "5px", scale: "-0.65 0.65"}} value={progress} />}</Box>}
                    {!effect.enabled && <Box/>}
                    {!selected && effect.enabled && <Button sx={{...classes.short, height: "100%"}} textalign={"center"} disabled={requestRunning} onClick={(e)=>{e.stopPropagation(); navigateTo(effectIndex);}}><Icon>play_circle_outline_arrow</Icon></Button>}
                </Box>
                
            </Box>
        }
        {open && <ConfigDialog heading={effect.name} effectIndex={effectIndex} open={open} setOpen={setOpen}></ConfigDialog>}
    </Card>;
};

Effect.propTypes = {
    effect: PropTypes.shape({
        name: PropTypes.string.isRequired,
        enabled: PropTypes.bool.isRequired
    }).isRequired,
    effectIndex: PropTypes.number.isRequired,
    effectEnable: PropTypes.func.isRequired,
    navigateTo: PropTypes.func.isRequired,
    requestRunning: PropTypes.bool.isRequired,
    gridLayout: PropTypes.bool.isRequired,
    onDragStart: PropTypes.func.isRequired,
    onDragOver: PropTypes.func.isRequired,
};

export default Effect;