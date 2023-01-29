const StaticStatsPanel = withStyles(statsStyle)(props => {
    const { classes, stat, name, detail } = props;

    return <Box className={classes.root}>
        <Typography variant={detail ? "h4" : "h7"}>{name}</Typography>
        {detail && <List>
            {Object.entries(stat.stat)
                   .map(entry=>
                <ListItem key={entry[0]}>
                    <Typography variant="littleHeader">{entry[0]}</Typography>:
                    <Typography variant="littleValue" >{entry[1]}</Typography>
                </ListItem>)}
        </List>}
    </Box>
});