const Effect = withStyles(effectStyle)(props => {
    const { classes, effect, effectInterval, effectIndex, timeRemaining, selected, postUpdate } = props;
    const [ progress, setProgress ] = useState(undefined);

    useEffect(()=>{
        const timeout = setTimeout(() => selected && setProgress((timeRemaining/effectInterval)*100.0),300);
        return () => clearTimeout(timeout);
    },[progress,selected]);

    useEffect(()=>{selected && setProgress((timeRemaining/effectInterval)*100.0)},[effect]);

    const navigateTo = (idx)=>{
        fetch(`${httpPrefix !== undefined ? httpPrefix : ""}/setCurrentEffectIndex?`,{method:"POST", body: new URLSearchParams({currentEffectIndex:idx})})
        .then(postUpdate)
        .catch(console.error);
    }

    const effectEnable = (idx,enable)=>{
        fetch(`${httpPrefix !== undefined ? httpPrefix : ""}/${enable?"enable":"disable"}Effect`,{method:"POST", body:new URLSearchParams({effectIndex:idx})})
        .then(postUpdate)
        .catch(console.error);
    }

    return <Box className={classes.effect}>
                <Box className={selected && classes.selected}>
                    <Box className={classes.effectPannel}>
                        {selected ?
                        <Box>
                            <Icon>arrow_right_alt</Icon>
                        </Box>:
                        <IconButton onClick={()=>effect.enabled && navigateTo(effectIndex)}><Icon className={classes.unselected}>{effect.enabled ? "arrow_right_alt":""}</Icon></IconButton>}
                        <IconButton onClick={()=>effectEnable(effectIndex,!effect.enabled)}><Icon>{effect.enabled ? "check" : "close"}</Icon></IconButton>
                    </Box>
                    <Box className={`${!effect.enabled && classes.unselected} ${classes.effectName}`}>{effect.name}</Box>
                </Box>
                {selected && <LinearProgress variant="determinate" sx={{transition: 'none'}} value={progress}/>}
            </Box>
});