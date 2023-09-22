import { useState, useEffect } from "react";
import { Box, Typography } from "@mui/material";
import countdownStyle from "./style";

const Countdown = props => {
    const {  label, millisecondsRemaining, requestRefresh } = props;
    const [ timeRemaining, setTimeRemaining ] = useState(false);

    useEffect(() => {
        if (millisecondsRemaining) {
            const timeReference = Date.now()+millisecondsRemaining;
            setTimeRemaining(timeReference-Date.now());
            var requestSent = false;
            const interval = setInterval(()=>{
                const remaining = timeReference-Date.now();
                if (remaining >= 0) {
                    setTimeRemaining(remaining);
                }
                if ((remaining <= 100) && !requestSent) {
                    requestSent=true;
                    requestRefresh();
                }
            },50);
            return ()=>clearInterval(interval);
        }
    },[millisecondsRemaining, requestRefresh]);

    return (            
        <Box sx={countdownStyle.root}>
            <Typography variant="little" color="textPrimary">{label}</Typography>:
            <Typography color="textSecondary" sx={countdownStyle.timeremaining} variant="little">{timeRemaining}</Typography>
        </Box>)

};

export default Countdown;