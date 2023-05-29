const Effect = withStyles(effectStyle)(props => {
    const [service] = useState(eventManager());

    const { classes, effect, effectInterval, millisecondsRemaining, selected } = props;
    const [ progress, setProgress ] = useState();
    const [expanded, setExpanded] = React.useState(false); 

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

    return <Card variant="outlined" className={[classes.effect,effect.enabled?null:classes.disabled]}>
                <CardHeader
                    avatar={
                        <Avatar aria-label={effect.name}>
                            {effect.name[0]}
                        </Avatar>
                      }
                    title={effect.name}
                    subheader={effect.enabled?(selected?"Active":"") : "Disabled"}
                    className={classes.cardheader}
                /> 
                <CardContent>
                    {selected && 
                        <div className={classes.circularProgress}>
                            <CircularProgress variant="determinate" value={progress} color="text.primary"/>
                            <Typography className={classes.circularProgressText} color="textSecondary" variant="little">{Math.floor(progress)}</Typography>
                        </div>}
                    {!selected && <IconButton color="secondary" onClick={()=>service.emit("toggleEffect",effect)}>{<Icon>{effect.enabled?"block":"add_alarm"}</Icon>}</IconButton >}
                    {!selected && effect.enabled && <IconButton color="secondary" onClick={()=>service.emit("navigateTo",effect)}><Icon>play_circle_outline</Icon></IconButton>}
                </CardContent>
                <CardActions disableSpacing>
                    <IconButton
                        onClick={()=>setExpanded(!expanded)}
                        aria-label="show more">
                        <Icon>settings</Icon>
                    </IconButton>
                </CardActions>
                <Collapse in={expanded} timeout="auto" unmountOnExit>
                    <CardContent>
                        <TextField label="Option"/>
                    </CardContent>
                </Collapse>
            </Card>
});