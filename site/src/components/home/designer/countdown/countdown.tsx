import { Box, Typography } from "@mui/material";
import { useState, useEffect } from "react";

interface ICountDownProps {
        label: string;
        millisecondsRemaining: number;
        requestRefresh:Function;
}

export function Countdown({ label, millisecondsRemaining, requestRefresh }:ICountDownProps) {
    const [ timeRemaining, setTimeRemaining ] = useState(0);

    useEffect(() => {
        if (millisecondsRemaining) {
            const timeReference = Date.now()+millisecondsRemaining;
            setTimeRemaining(timeReference-Date.now());
            let requestSent = false;
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
    },[millisecondsRemaining]);

    return (            
    <Box>
        <Typography variant="body2" color="textPrimary">{label}</Typography>:
        <Typography color="textSecondary" width="100px" variant="caption">{timeRemaining}</Typography>
    </Box>)

}