const DesignerPanel = withStyles(designStyle)(props => {
    const [chipConfig, setChipConfig] = useState();
    const [service] = useState(eventManager());

    const { classes, open } = props;
    const [ effects, setEffects ] = useState();
    const [ nextRefreshDate, setNextRefreshDate] = useState();
    const [ editing, setEditing ] = useState(false);
    const [ effectInterval, setEffectInterval ] = useState(effects && effects.effectInterval);
    const [ hoverEffect, setHoverEffect ] = useState(undefined);
    const [ displayMode, setDisplayMode ] = useState( props.displayMode );
    const [ progress, setProgress ] = useState(99);
    

    useEffect(() => {
        if (effects && effects.millisecondsRemaining) {
            const timeReference = Date.now()+effects.millisecondsRemaining;
            let timeRemaining = timeReference-Date.now();
            const interval = setInterval(()=>{
                const remaining = timeReference-Date.now();
                if (remaining >= 0) {
                    timeRemaining = remaining;
                    setProgress((1-timeRemaining/effects.effectInterval)*100.0);
                }
            },300);
            return ()=>clearInterval(interval);
        }
    },[effects ? effects.millisecondsRemaining:0]);

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
    <Card variant="outlined" className={`${!open && classes.hidden}`}>
        <CardHeader 
                action={<IconButton aria-label="Next" onClick={()=>setDisplayMode(displayMode === "detailed" ? "summary":"detailed")}>
                    <Icon>{displayMode === "detailed" ? "expand_less":"expand_more"}</Icon></IconButton>}
                title={`${effects.Effects.length} effects`}
                subheader={editing?editingHeader():displayHeader()} />
        <CardContent>
            <Box className={displayMode === "summary" ? classes.summaryEffects : classes.effects}>
                {effects.Effects.map((effect,idx) => 
                    <Box key={`effect-${idx}`} onMouseEnter={()=>{setHoverEffect(effect)}}
                        onMouseLeave={()=>{setHoverEffect(undefined)}}>
                        <Effect
                            effect={effect}
                            displayMode={displayMode}
                            effectInterval={effects.effectInterval}
                            selected={idx === effects.currentEffect}
                            millisecondsRemaining={effects.millisecondsRemaining}/>
                    </Box>)}
            </Box>
            {hoverEffect?<Typography variant="tiny">{hoverEffect.name}</Typography>:<Typography variant="tiny">{effects.Effects[effects.currentEffect].name}</Typography>}
        </CardContent>
        <LinearProgress variant="determinate" aria-label={Math.floor(progress)} value={progress} />
        <CardActions disableSpacing>
            <IconButton aria-label="Previous" onClick={()=>service.emit("navigate",false)}><Icon>skip_previous</Icon></IconButton>
            <IconButton aria-label="Next" onClick={()=>service.emit("navigate",true)}><Icon>skip_next</Icon></IconButton>
            <IconButton aria-label="Refresh Effects" onClick={()=>setNextRefreshDate(Date.now())}><Icon>refresh</Icon></IconButton>
        </CardActions>
    </Card>
});