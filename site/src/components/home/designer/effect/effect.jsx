const Effect = withStyles(effectStyle)(props => {
    const { classes, effect, effectInterval, effectIndex, millisecondsRemaining, selected, effectEnable, navigateTo, requestRunning } = props;
    const [ progress, setProgress ] = useState(undefined);
    const [expanded, setExpanded] = React.useState(false);

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

    return <Card variant="outlined" className={classes.effect}>
                <CardHeader
                    avatar={
                        <Avatar aria-label={effect.name}>
                            {effect.name[0]}
                        </Avatar>
                      }
                    title={effect.name}
                    subheader={effect.enabled?(selected?"Active":"Waiting") : "Disabled"}
                    className={classes.cardheader}
                /> 
                <CardContent>
                    {selected && <LinearProgress disabled={requestRunning} variant="determinate" sx={{transition: 'none'}} value={progress}/>}
                    {!selected && <Button disabled={requestRunning} onClick={()=>effectEnable(effectIndex,!effect.enabled)} variant="outlined" startIcon={<Icon >{effect.enabled?"stop":"circle"}</Icon>}>{effect.enabled?"Disable":"Enable"}</Button>}
                    {!selected && effect.enabled && <Button disabled={requestRunning} onClick={()=>navigateTo(effectIndex)} variant="outlined" startIcon={<Icon >start</Icon>}>Trigger</Button>}
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