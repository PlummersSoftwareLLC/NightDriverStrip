const DesignerPanel = withStyles(designStyle)(props => {
    const { classes, open, addNotification } = props;
    const [ effects, setEffects ] = useState(undefined);
    const [ abortControler, setAbortControler ] = useState(undefined);
    const [ nextRefreshDate, setNextRefreshDate] = useState(undefined);
    const [ editing, setEditing ] = useState(false);
    const [ requestRunning, setRequestRunning ] = useState(false);
    const [ pendingInterval, setPendingInterval ] = useState(undefined);

    useEffect(() => {
        if (abortControler) {
            abortControler.abort();
        }

        if (open) {
            const aborter = new AbortController();
            setAbortControler(aborter);

            const timer = setTimeout(()=>{
                setNextRefreshDate(Date.now());
            },3000);
    
            chipRequest(`${httpPrefix !== undefined ? httpPrefix : ""}/getEffectList`,{signal:aborter.signal})
                .then(resp => resp.json())
                .then(setEffects)
                .then(()=>clearTimeout(timer))
                .catch(err => addNotification("Error","Service","Get Effect List",err));
    
            return () => {
                abortControler && abortControler.abort();
                clearTimeout(timer);
            }
        }
    },[open,nextRefreshDate]);

    const requestRefresh = () => setTimeout(()=>setNextRefreshDate(Date.now()),50);

    const chipRequest = (url,options,operation) => {
        setRequestRunning(true);
        return fetch(url,options)
                .catch(err => addNotification("Error",operation,err))
                .finally(()=>setRequestRunning(false));
    };

    const navigateTo = (idx)=>{
        return chipRequest(`${httpPrefix !== undefined ? httpPrefix : ""}/setCurrentEffectIndex?`,{method:"POST", body: new URLSearchParams({currentEffectIndex:idx})})
                .then(requestRefresh);
    }

    const effectEnable = (idx,enable)=>{
        return chipRequest(`${httpPrefix !== undefined ? httpPrefix : ""}/${enable?"enable":"disable"}Effect`,{method:"POST", body:new URLSearchParams({effectIndex:idx})})
        .then(requestRefresh);
    }

    const navigate = (up)=>{
        return chipRequest(`${httpPrefix !== undefined ? httpPrefix : ""}/${up ? "nextEffect" : "previousEffect"}`,{method:"POST"})
        .then(requestRefresh);
    }

    const updateEventInterval = (interval)=>{
        const abort = new AbortController();
        const timer = setTimeout(()=>abort.abort(),3000);
        chipRequest(`${httpPrefix !== undefined ? httpPrefix : ""}/settings`,
        {
            method:"POST",
            signal:abort.signal,
            body: new URLSearchParams({effectInterval:interval})
        }).then(()=>clearTimeout(timer))
          .then(requestRefresh)
          .catch(_err => clearTimeout(timer));
    }

    const displayHeader = ()=>{
        return <Box className={classes.effectsHeaderValue}>
            <Typography variant="little" color="textSecondary">Interval</Typography>:
            <Typography variant="little" color="textAttribute">{effects.effectInterval}</Typography>
            <IconButton onClick={() => setEditing(true)}><Icon>edit</Icon></IconButton>
        </Box>;
    }

    const editingHeader = ()=>{
        return <Box className={classes.effectsHeaderValue}>
            <TextField label="Interval ms"
                variant="outlined"
                type="number"
                defaultValue={effects.effectInterval}
                onChange={event => setPendingInterval(event.target.value)} />
            <Box className={classes.saveIcons}>
                <IconButton color="primary"
                    aria-label="Save"
                    component="label"
                    onClick={_evt => {
                        updateEventInterval(pendingInterval);
                        setEditing(false);
                    } }>
                    <Icon>save</Icon>
                </IconButton>
                <IconButton color="secondary"
                    aria-label="Cancel"
                    component="label"
                    onClick={_evt => {
                        setEditing(false);
                    } }>
                    <Icon>cancel</Icon>
                </IconButton>
            </Box>
        </Box>;
    }

    if (!effects && open){
        return <Box>Loading....</Box>;
    }

    return effects && <Box className={`${classes.root} ${!open && classes.hidden}`}>
        <Box className={classes.effectsHeader}>
            {editing ? 
            editingHeader():
            displayHeader()}
            <Countdown 
                label="Time Remaining"
                requestRefresh={requestRefresh}
                millisecondsRemaining={effects.millisecondsRemaining}/>
            {(effects.Effects.length > 1) && <Box>
                <IconButton disabled={requestRunning} onClick={()=>navigate(false)}><Icon>skip_previous</Icon></IconButton>
                <IconButton disabled={requestRunning} onClick={()=>navigate(true)}><Icon>skip_next</Icon></IconButton>
                <IconButton disabled={requestRunning} onClick={()=>setNextRefreshDate(Date.now())}><Icon>refresh</Icon></IconButton>
            </Box>}
        </Box>
        <Box className={classes.effects}>
            {effects.Effects.map((effect,idx) => <Effect 
                                                    key={`effect-${idx}`}
                                                    effect={effect} 
                                                    effectIndex={idx} 
                                                    navigateTo={navigateTo}
                                                    requestRunning={requestRunning}
                                                    effectEnable={effectEnable}
                                                    effectInterval={effects.effectInterval}
                                                    selected={idx === effects.currentEffect}
                                                    millisecondsRemaining={effects.millisecondsRemaining}/>)}
        </Box>
    </Box>
});