const Effect = withStyles(effectStyle)(props => {
    const { classes, effect, effectInterval, effectIndex, millisecondsRemaining, selected, effectEnable, navigateTo, requestRunning } = props;
    const [ progress, setProgress ] = useState(undefined);

    useEffect(() => {
        if (millisecondsRemaining && selected) {
            const timeReference = Date.now()+millisecondsRemaining;
            var timeRemaining = timeReference-Date.now();
            const interval = setInterval(()=>{
                const remaining = timeReference-Date.now();
                if (remaining >= 0) {
                    timeRemaining = remaining;
                    setProgress((timeRemaining/effectInterval)*100.0);
                }
            },300);
            return ()=>clearInterval(interval);
        }
    },[millisecondsRemaining,selected]);

    return <Box className={classes.effect}>
                <Box className={`${selected && classes.selected} ${classes.effectPannel}`}>
                    <Box className={`${!effect.enabled && classes.unselected} ${classes.effectName}`}>{effect.name}</Box>
                    {selected ?
                    <Box>
                        <Icon>arrow_right_alt</Icon>
                    </Box>:
                    <IconButton disabled={requestRunning} onClick={()=>effect.enabled && navigateTo(effectIndex)}><Icon className={classes.unselected}>{effect.enabled ? "arrow_right_alt":""}</Icon></IconButton>}
                    <IconButton disabled={requestRunning} onClick={()=>effectEnable(effectIndex,!effect.enabled)}><Icon>{effect.enabled ? "check" : "close"}</Icon></IconButton>
                </Box>
                {selected && <LinearProgress variant="determinate" sx={{transition: 'none'}} value={progress}/>}
            </Box>
});