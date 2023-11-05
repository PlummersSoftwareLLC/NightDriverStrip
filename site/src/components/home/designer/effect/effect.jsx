import {useState, useEffect, useContext} from 'react';
import {IconButton, Icon, Card, CardHeader, CardContent, Avatar, CardActions, Box } from '@mui/material';
import {LinearProgress, Button, useTheme} from '@mui/material';
import effectStyle from './style';
import PropTypes from 'prop-types';
import ConfigDialog from '../../config/configDialog';
import { EffectsContext } from '../../../../context/effectsContext';

const Effect = props => {
    const {activeInterval,remainingInterval, pinnedEffect, currentEffect} = useContext(EffectsContext);
    const { effect, effectIndex, effectEnable, navigateTo, requestRunning } = props;
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

    return <Card variant="outlined" sx={classes.effect}>
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
};

export default Effect;