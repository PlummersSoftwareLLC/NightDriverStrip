const StaticStatsPanel = withStyles(staticStatStyle)(props => {
    const { classes, stat, name, detail } = props;

    return <Box className={classes.root}>
        <Typography >{name}</Typography>
        {detail ? <List>
            {Object.entries(stat.stat)
                   .map(entry=>
                <ListItem key={entry[0]}>
                    <Typography variant="little" color="textPrimary">{entry[0]}</Typography>:
                    <Typography variant="little" color="textSecondary">{entry[1]}</Typography>
                </ListItem>)}
        </List>:
        <List>
        {Object.entries(stat.stat)
               .filter(entry => stat.headerFields.includes(entry[0]))
               .map(entry=><ListItem key={entry[0]}><Typography variant="little" color="textSecondary" >{entry[1]}</Typography></ListItem>)}
        </List>}
    </Box>
});