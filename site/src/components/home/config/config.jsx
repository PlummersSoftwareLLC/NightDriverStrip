const ConfigDialog = withStyles(configStyle)(props => {
  const { classes, open, onClose, siteConfig } = props;
  return (
    <Dialog
      fullScreen
      open={open}
      onClose={() => onClose && onClose()}>
      <AppBar sx={{ position: 'relative' }}>
        <Toolbar>
          <IconButton
            edge="start"
            color="inherit"
            onClick={()=>onClose && onClose()}
            aria-label="close">
            <Icon>close</Icon>
          </IconButton>
          <Typography sx={{ ml: 2, flex: 1 }} variant="h6" component="div">
            Configuration
          </Typography>
        </Toolbar>
      </AppBar>
      <List className={classes.configBar}>
        <ListItem>
          <List>
            <ListItemText primary="Site Configuration"/>
            <List>
                {Object.entries(siteConfig).map(entry => <ConfigItem 
                                            name={entry[1].name}
                                            key={entry[1].name}
                                            datatype={entry[1].type}
                                            value={entry[1].value}
                                            configItemUpdated={value => entry[1].setter(value)} 
                                            />)}
            </List>
          </List>
        </ListItem>
        <Divider />
      </List>
    </Dialog>
  );
});