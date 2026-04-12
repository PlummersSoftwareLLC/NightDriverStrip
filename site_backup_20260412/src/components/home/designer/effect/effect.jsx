import {useState, useEffect, useContext} from 'react';
import {IconButton, Icon, Card, CardHeader, CardContent, Avatar, CardActions, Box, Checkbox} from '@mui/material';
import {LinearProgress, Button, useTheme} from '@mui/material';
import effectStyle from './style';
import PropTypes from 'prop-types';
import ConfigDialog from '../../config/configDialog';
import { EffectsContext } from '../../../../context/effectsContext';

const ListLayout = ({classes, setOpen, effect, selected, effectEnable, effectIndex, progress, requestRunning, navigateTo, pinnedEffect}) => {
    return <Box sx={{display: "flex", height: "100%"}} flexDirection={"row"}>
        <Box sx={{float: 'left', textAlign: "left", display: 'flex'}} flexDirection={"row"}>
            <Checkbox checked={effect.enabled} disabled={selected} onClick={(e)=>{e.stopPropagation(); effectEnable(effectIndex,!effect.enabled);}} sx={classes.short}/>
            <CardHeader sx={classes.short}
                avatar={
                    <Avatar aria-label={effect.name}>
                        {effect.name[0]}
                    </Avatar>
                }
            /> 
        </Box>
        <Box sx={{float:'left', textAlign: "center", height:'100%'}} flexGrow={1}>
            <Box sx={{ alignItems:'center', display: 'flex',flexDirection:'column', height:'100%'}}>
                <Box flexGrow={1}>
                    {effect.name}

                </Box>
                <Box flexGrow={1} sx={{maxWidth: '400px', width:'100%'}}>
                    {selected && (pinnedEffect 
                        ? <Box sx={{textAlign: 'center'}}><Icon>all_inclusive</Icon></Box> 
                        : <LinearProgress disabled={requestRunning} variant="determinate" sx={{transition: 'none'}} value={progress} />
                    )}
                </Box>
            </Box>
        </Box>
        <Box sx={{float: 'left', textAlign: "-moz-right"}}>
            <Box sx={{float: 'left', width: '50%', height:'100%'}}>
                {(!effect.enabled || selected) && <Box sx={{width: '40px'}}/>}
                {!selected && effect.enabled && <IconButton sx={{height: '100%'}} disabled={requestRunning} onClick={()=>navigateTo(effectIndex)}><Icon>play_circle_outline_arrow</Icon></IconButton>}
            </Box>
            <Box sx={{float: 'left', width: '50%', height:'100%'}}>
                <IconButton sx={{height: '100%'}}
                    onClick={()=>setOpen(true)}
                    aria-label="show more">
                    <Icon>settings</Icon>
                </IconButton>
            </Box>
        </Box>
    </Box>
}

const GridLayout = ({classes, setOpen, effect, selected, effectEnable, effectIndex, progress, requestRunning, navigateTo, pinnedEffect}) => {
    return <>
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
}

const Effect = props => {
    const {activeInterval,remainingInterval, pinnedEffect, currentEffect} = useContext(EffectsContext);
    const { effect, effectIndex, effectEnable, navigateTo, requestRunning, gridLayout, onDragStart, onDragOver} = props;
    const [ progress, setProgress ] = useState(0);
    const [open, setOpen] = useState(false);
    const selected = Number(effectIndex) === currentEffect;
    const theme = useTheme();
    const classes = effectStyle(theme);
    const CardLayout = gridLayout ? GridLayout : ListLayout
    
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
    ><CardLayout 
            classes={classes}
            setOpen={setOpen}
            effect={effect}
            selected={selected}
            effectEnable={effectEnable} 
            effectIndex={effectIndex} 
            progress={progress} 
            requestRunning={requestRunning}
            navigateTo={navigateTo}
            pinnedEffect={pinnedEffect}
        />
        {open && <ConfigDialog heading={effect.name} effectIndex={effectIndex} open={open} setOpen={setOpen}></ConfigDialog>}
    </Card>;
};

Effect.propTypes = {
    effect: PropTypes.shape({
        name: PropTypes.string.isRequired,
    }).isRequired,
    effectIndex: PropTypes.number.isRequired,
    effectEnable: PropTypes.func.isRequired,
    navigateTo: PropTypes.func.isRequired,
    requestRunning: PropTypes.bool.isRequired,
    gridLayout: PropTypes.bool.isRequired,
    onDragStart: PropTypes.func.isRequired,
    onDragOver: PropTypes.func.isRequired,
};

const layoutProps = {
    classes: PropTypes.shape({
        short: PropTypes.shape({}).isRequired, 
        cardheader: PropTypes.shape({}).isRequired
    }).isRequired,
    setOpen: PropTypes.func.isRequired,
    effect: PropTypes.shape({
        name: PropTypes.string.isRequired, 
        enabled: PropTypes.bool.isRequired
    }).isRequired,
    selected: PropTypes.bool.isRequired,
    effectEnable: PropTypes.func.isRequired, 
    effectIndex: PropTypes.number.isRequired, 
    progress: PropTypes.number.isRequired, 
    requestRunning: PropTypes.bool.isRequired, 
    navigateTo: PropTypes.func.isRequired, 
    pinnedEffect:PropTypes.bool.isRequired
};

ListLayout.propTypes = layoutProps
GridLayout.propTypes = layoutProps

export default Effect;