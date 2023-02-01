const DesignerPanel = withStyles(designStyle)(props => {
    const { classes, siteConfig, open } = props;
    const [ effects, setEffects ] = useState(undefined);
    const [ abortControler, setAbortControler ] = useState(undefined);
    const [ nextRefreshDate, setNextRefreshDate] = useState(undefined);
    const [ editing, setEditing ] = useState(false);
    const [ pendingInterval, setPendingInterval ] = useState(undefined);
    const [ timeRemaining, setTimeRemaining ] = useState(undefined);

    useEffect(() => {
        if (abortControler) {
            abortControler.abort();
        }

        if (open) {
            const aborter = new AbortController();
            setAbortControler(aborter);
    
            fetch(`${httpPrefix !== undefined ? httpPrefix : ""}/getEffectList`,{signal:aborter.signal})
                .then(resp => resp.json())
                .then(setEffects)
                .catch(console.error);
    
            return () => {
                abortControler && abortControler.abort();
            }
        }
    },[open,nextRefreshDate]);

    useEffect(() => {
        if (effects) {
            const timeReference = Date.now()+effects.millisecondsRemaining;
            var requestSent = false;
            const interval = setInterval(()=>{
                const remaining = timeReference-Date.now();
                if (remaining >= 0) {
                    setTimeRemaining(remaining);
                }
                if ((remaining <= 100) && !requestSent) {
                    setNextRefreshDate(Date.now());
                    requestSent=true;
                }
            },50);
            return ()=>clearInterval(interval);
        }
    },[effects]);

    const postUpdate = () => setTimeout(()=>setNextRefreshDate(Date.now()),50);

    const navigate = (up)=>{
        fetch(`${httpPrefix !== undefined ? httpPrefix : ""}/${up ? "nextEffect" : "previousEffect"}`,{method:"POST"})
        .then(postUpdate)
        .catch(console.error);
    }

    const navigateTo = (idx)=>{
        fetch(`${httpPrefix !== undefined ? httpPrefix : ""}/setCurrentEffectIndex?`,{method:"POST", body: new URLSearchParams({currentEffectIndex:idx})})
        .then(postUpdate)
        .catch(console.error);
    }

    const updatePendingInterval = (interval)=>{
        fetch(`${httpPrefix !== undefined ? httpPrefix : ""}/settings`,{method:"POST", body: new URLSearchParams({effectInterval:interval})})
        .then(postUpdate)
        .catch(console.error);
    }

    const effectEnable = (idx,enable)=>{
        fetch(`${httpPrefix !== undefined ? httpPrefix : ""}/${enable?"enable":"disable"}Effect`,{method:"POST", body:new URLSearchParams({effectIndex:idx})})
        .then(postUpdate)
        .catch(console.error);
    }

    if (!effects && open){
        return <Box>Loading....</Box>;
    }

    return effects && <Box className={`${classes.root} ${!open && classes.hidden}`}>
        <Box className={classes.effectsHeader}>
            {editing ? 
            <Box className={classes.effectsHeaderValue}>
                <TextField label="Interval ms" 
                           variant="outlined"
                           type="number"
                           defaultValue={effects.effectInterval}
                           onChange={event => setPendingInterval(event.target.value) } />
                <Box className={classes.saveIcons}>
                    <IconButton color="primary" 
                                aria-label="Save" 
                                component="label"
                                onClick={_evt => {
                                    updatePendingInterval(pendingInterval);
                                    setEditing(false);
                                }}>
                        <Icon>save</Icon>
                    </IconButton>
                    <IconButton color="secondary" 
                                aria-label="Cancel" 
                                component="label"
                                onClick={_evt => {
                                    setEditing(false);
                                }}>
                        <Icon>cancel</Icon>
                    </IconButton>
                </Box>
            </Box>:
            <Box className={classes.effectsHeaderValue}>
                <Typography variant="little" color="textSecondary">Interval</Typography>:
                <Typography variant="little" color="textAttribute">{effects.effectInterval}</Typography>
                <IconButton onClick={()=>setEditing(true)}><Icon>edit</Icon></IconButton>
            </Box>}
            <Box className={classes.effectsHeaderValue}>
                <Typography variant="little" color="textSecondary">Time Remaining</Typography>:
                <Typography className={classes.timeremaining} width="100px" variant="little" color="textAttribute">{timeRemaining}</Typography>
            </Box>
            {(effects.Effects.length > 1) && <Box>
                <IconButton onClick={()=>navigate(false)}><Icon>skip_previous</Icon></IconButton>
                <IconButton onClick={()=>navigate(true)}><Icon>skip_next</Icon></IconButton>
                <IconButton onClick={()=>setNextRefreshDate(Date.now())}><Icon>refresh</Icon></IconButton>
            </Box>}
        </Box>
        <Box className={classes.effects}>
            {effects.Effects.map((effect,idx) => 
                <Box key={`effect-${idx}`} className={classes.effect}>
                    {(idx === effects.currentEffect) ?
                    effects.Effects.length > 1 && <Icon>arrow_right_alt</Icon>:
                    <IconButton onClick={()=>effect.enabled && navigateTo(idx)}><Icon className={classes.unselected}>{effect.enabled ? "arrow_right_alt":""}</Icon></IconButton>}
                    <Box className={`${classes.effectAttribute} ${(idx === effects.currentEffect) && classes.selected}`}>
                        <IconButton onClick={()=>effectEnable(idx,!effect.enabled)}><Icon>{effect.enabled ? "check" : "close"}</Icon></IconButton>
                        <Box className={!effect.enabled && classes.unselected}>{effect.name}</Box>
                    </Box>
                </Box>)}
        </Box>
    </Box>
});