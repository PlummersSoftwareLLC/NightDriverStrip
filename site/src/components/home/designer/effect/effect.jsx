const Effect = withStyles(effectStyle)(props => {
    const [service] = useState(eventManager());

    const { classes, effect, effectInterval, millisecondsRemaining, selected, displayMode } = props;
    const [ progress, setProgress ] = useState(0);

    useEffect(() => {
        if (millisecondsRemaining && selected) {
            const timeReference = Date.now()+millisecondsRemaining;
            let timeRemaining = timeReference-Date.now();
            const interval = setInterval(()=>{
                const remaining = timeReference-Date.now();
                if (remaining >= 0) {
                    timeRemaining = remaining;
                    setProgress((timeRemaining/effectInterval)*100.0);
                }
            },300);
            return ()=>clearInterval(interval);
        }
        if (!selected) {
            setProgress(99);
        }
    },[millisecondsRemaining,selected]);

    if (displayMode === "summary") {
        return <div className={`${classes.dot} ${selected ? classes.selected:classes.waiting}`} 
                    onClick={() => service.emit("navigateTo", effect)}/>;
    } 
    return <Card variant="outlined" className={[
            classes.effect,
            effect.enabled ? null : classes.disabled,
            selected ? classes.playing : null
        ]}><CardHeader
                avatar={<Avatar aria-label={effect.name}>
                    {effect.name[0]}
                </Avatar>}
                title={effect.name}
                subheader={effect.enabled ? (selected ? "Active" : "") : "Disabled"}
                className={classes.cardheader} />
            <CardContent className={classes.cardcontent}>
                {selected &&
                    <div className={classes.circularProgress}>
                        <CircularProgress aria-label={Math.floor(progress)} variant="determinate" value={progress} color="text.primary" />
                        <Typography className={classes.circularProgressText} color="textSecondary" variant="little">{Math.floor(progress)}</Typography>
                    </div>}
                {!selected && <IconButton aria-label="Toggle Effect" color="secondary" onClick={() => service.emit("toggleEffect", effect)}>{<Icon>{effect.enabled ? "block" : "add_alarm"}</Icon>}</IconButton>}
                {!selected && effect.enabled && <IconButton aria-label="Select Effect" color="secondary" onClick={() => service.emit("navigateTo", effect)}><Icon>play_circle_outline</Icon></IconButton>}
            </CardContent>
        </Card>;
    }
);