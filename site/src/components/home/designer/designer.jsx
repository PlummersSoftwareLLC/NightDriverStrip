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
                action={<IconButton aria-label="Next" onClick={()=>setDisplayMode(displayMode === "detailed" ? "summary":"detailed")}><Icon>{displayMode === "detailed" ? "minimize":"maximize"}</Icon></IconButton>}
                title={`${effects.Effects.length} effects`}
                subheader={editing?editingHeader():displayHeader()} />
        <CardContent>
            <Box className={displayMode === "summary" ? classes.summaryEffects : classes.effects}>
                {effects.Effects.map((effect,idx) => 
                    <div onMouseEnter={()=>{setHoverEffect(effect)}}
                        onMouseLeave={()=>{setHoverEffect(undefined)}}>
                        <Effect key={`effect-${idx}`}
                            effect={effect}
                            displayMode={displayMode}
                            effectInterval={effects.effectInterval}
                            selected={idx === effects.currentEffect}
                            millisecondsRemaining={effects.millisecondsRemaining}/>
                    </div>)}
            </Box>
            {hoverEffect?<Typography variant="tiny">{hoverEffect.name}</Typography>:<br/>}
        </CardContent>
        <CardActions disableSpacing>
            <IconButton aria-label="Previous" onClick={()=>service.emit("navigate",false)}><Icon>skip_previous</Icon></IconButton>
            <IconButton aria-label="Next" onClick={()=>service.emit("navigate",true)}><Icon>skip_next</Icon></IconButton>
            <IconButton aria-label="Refresh Effects" onClick={()=>setNextRefreshDate(Date.now())}><Icon>refresh</Icon></IconButton>
        </CardActions>
    </Card>
});