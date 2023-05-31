const DesignerPanel = withStyles(designStyle)(props => {
    const [chipConfig, setChipConfig] = useState();
    const [service] = useState(eventManager());

    const { classes, open } = props;
    const [ effects, setEffects ] = useState();
    const [ nextRefreshDate, setNextRefreshDate] = useState();
    const [ editing, setEditing ] = useState(false);
    const [ effectInterval, setEffectInterval ] = useState(effects && effects.effectInterval);

    useEffect(() => {
        const subs={
            chipConfig:service.subscribe("ChipConfig",cfg=>{setChipConfig(cfg)}),
            effectsSub:service.subscribe("effectList",effectList=>{setEffects(effectList)})
        };
        
        return ()=>Object.values(subs).forEach(service.unsubscribe);
    }, [service]);

    useEffect(() => {
        if (open) {
            service.emit("refreshEffectList");
            const timer = setTimeout(()=>{
                setNextRefreshDate(Date.now());
            },3000);

            return () => clearTimeout(timer);
        }
    },[open,nextRefreshDate]);

    const requestRefresh = () => setNextRefreshDate(Date.now());

    const displayHeader = ()=>{
        return <Box className={classes.effectsHeaderValue}>
            <Typography variant="little" color="textPrimary">Interval</Typography>:
            <Link href="#" variant="little" color="textSecondary" onClick={() => setEditing(true)}>{effects.effectInterval}</Link>
        </Box>;
    };

    const editingHeader = ()=>{
        return <ClickAwayListener onClickAway={()=>{
                    service.emit("SetChipConfig",{...chipConfig,effectInterval});
                    setEditing(false);
                    setEffects((prev=>{return {...prev,effectInterval}}));
                    setNextRefreshDate(Date.now());
                    }}>
                    <Box className={classes.effectsHeaderValue}>
                        <TextField label="Interval ms"
                            variant="outlined"
                            type="number"
                            defaultValue={effects.effectInterval}
                            onChange={event => setEffectInterval(parseInt(event.target.value))} />
                    </Box>
                </ClickAwayListener>;
    };

    if (!effects && open){
        return <Box>Loading....</Box>;
    }

    return effects && effects.Effects && 
    <Box className={`${classes.root} ${!open && classes.hidden}`}>
        <Box className={classes.effectsHeader}>
            {editing ?
            editingHeader():
            displayHeader()}
            <Countdown
                label="Time Remaining"
                requestRefresh={requestRefresh}
                millisecondsRemaining={effects.millisecondsRemaining}/>
            {(effects !== undefined) && <Box>
                <IconButton aria-label="Previous" onClick={()=>service.emit("navigate",false)}><Icon>skip_previous</Icon></IconButton>
                <IconButton aria-label="Next" onClick={()=>service.emit("navigate",true)}><Icon>skip_next</Icon></IconButton>
                <IconButton aria-label="Refresh Effects" onClick={()=>setNextRefreshDate(Date.now())}><Icon>refresh</Icon></IconButton>
            </Box>}
        </Box>
        <Box className={classes.effects}>
            {effects.Effects.map((effect,idx) => 
                <Effect key={`effect-${idx}`}
                        effect={effect}
                        effectInterval={effects.effectInterval}
                        selected={idx === effects.currentEffect}
                        millisecondsRemaining={effects.millisecondsRemaining}/>)}
        </Box>
    </Box>
});