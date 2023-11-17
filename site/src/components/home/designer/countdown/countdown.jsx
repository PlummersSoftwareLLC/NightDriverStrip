import { useState, useEffect, useContext } from "react";
import { Box, Icon, Typography } from "@mui/material";
import countdownStyle from "./style";
import { EffectsContext } from "../../../../context/effectsContext";
import PropTypes from 'prop-types';

const Countdown = props => {
    const { remainingInterval, pinnedEffect, sync} = useContext(EffectsContext);
    const {  label } = props;
    const [ timeRemaining, setTimeRemaining ] = useState(false);
    useEffect(() => {
        if (!pinnedEffect && remainingInterval) {
            const timeReference = Date.now()+remainingInterval;
            setTimeRemaining(timeReference-Date.now());
            var requestSent = false;
            const interval = setInterval(()=>{
                const remaining = timeReference-Date.now();
                if (remaining >= 0) {
                    setTimeRemaining(remaining);
                }
                if ((remaining <= 100) && !requestSent) {
                    requestSent=true;
                    sync();
                }
            },50);
            return ()=>clearInterval(interval);
        }
    },[pinnedEffect, remainingInterval,sync]);

    return (            
        <Box sx={countdownStyle.root}> 
            <Typography variant="little" color="textPrimary">{label}</Typography>:
            <Typography color="textSecondary" sx={pinnedEffect? countdownStyle.pinned : countdownStyle.timeremaining} variant="little">{pinnedEffect ? <Icon>all_inclusive</Icon> : timeRemaining}</Typography>
        </Box>)

};

Countdown.propTypes = {
    label: PropTypes.string.isRequired
};

export default Countdown;