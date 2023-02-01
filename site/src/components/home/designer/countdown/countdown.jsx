const Countdown = withStyles(countdownStyle)(props => {
    const { classes,  label, millisecondsRemaining, postUpdate } = props;
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
                    postUpdate();
                }
            },50);
            return ()=>clearInterval(interval);
        }
    },[millisecondsRemaining]);

    return (            
    <Box className={classes.root}>
        <Typography variant="little" color="textSecondary">{label}</Typography>:
        <Typography className={classes.timeremaining} width="100px" variant="little" color="textAttribute">{timeRemaining}</Typography>
    </Box>)

});