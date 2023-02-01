const DesignerPanel = withStyles(designStyle)(props => {
    const { classes, open, addNotification } = props;
    const [ effects, setEffects ] = useState(undefined);
    const [ abortControler, setAbortControler ] = useState(undefined);
    const [ nextRefreshDate, setNextRefreshDate] = useState(undefined);
    const [ editing, setEditing ] = useState(false);
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
    
            fetch(`${httpPrefix !== undefined ? httpPrefix : ""}/getEffectList`,{signal:aborter.signal})
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

    const postUpdate = () => setTimeout(()=>setNextRefreshDate(Date.now()),50);

    const navigate = (up)=>{
        fetch(`${httpPrefix !== undefined ? httpPrefix : ""}/${up ? "nextEffect" : "previousEffect"}`,{method:"POST"})
        .then(postUpdate)
        .catch(err => addNotification("Error","Service","Effect Navigation",err));
    }

    const updateEventInterval = (interval)=>{
        fetch(`${httpPrefix !== undefined ? httpPrefix : ""}/settings`,
        {
            method:"POST", 
            body: new URLSearchParams({effectInterval:interval}),
            timeout: 3000
        })
        .then(postUpdate)
        .catch(err => addNotification("Error","Service","Update Settings",err));
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
                postUpdate={postUpdate}
                millisecondsRemaining={effects.millisecondsRemaining}/>
            {(effects.Effects.length > 1) && <Box>
                <IconButton onClick={()=>navigate(false)}><Icon>skip_previous</Icon></IconButton>
                <IconButton onClick={()=>navigate(true)}><Icon>skip_next</Icon></IconButton>
                <IconButton onClick={()=>setNextRefreshDate(Date.now())}><Icon>refresh</Icon></IconButton>
            </Box>}
        </Box>
        <Box className={classes.effects}>
            {effects.Effects.map((effect,idx) => <Effect 
                                                    key={`effect-${idx}`}
                                                    effect={effect} 
                                                    effectIndex={idx} 
                                                    postUpdate={postUpdate}
                                                    effectInterval={effects.effectInterval}
                                                    selected={idx === effects.currentEffect}
                                                    millisecondsRemaining={effects.millisecondsRemaining}/>)}
        </Box>
    </Box>
});