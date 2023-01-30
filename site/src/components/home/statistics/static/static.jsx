const StaticStatsPanel = withStyles(staticStatStyle)(props => {
    const { classes, stat, name, detail } = props;

    return <Box className={classes.root}>
        <Typography variant={detail ? "h5" : "h6"}>{name}</Typography>
        {detail ? <List>
            {Object.entries(stat.stat)
                   .map(entry=>
                <ListItem key={entry[0]}>
                    <Typography variant="littleHeader">{entry[0]}</Typography>:
                    <Typography className={classes.attribute} variant="littleValue" >{entry[1]}</Typography>
                </ListItem>)}
        </List>:
        <List>
        {Object.entries(stat.stat)
               .filter(entry => stat.headerFields.includes(entry[0]))
               .map(entry=><Typography className={classes.attribute} variant="littleValue" >{entry[1]}</Typography>)}
    </List>}
    </Box>
});