const Transition = React.forwardRef(function Transition(props, ref) {
  return <Slide direction="up" ref={ref} {...props} />;
});

const ConfigDialog = withStyles(configStyle)(props => {
  const [open, setOpen] = React.useState(true);
  const { classes, onClose, siteConfig } = props;

  const handleClose = () => {
    setOpen(false);
    onClose && onClose();
  };

  return (
    <div>
      <Dialog
        fullScreen
        open={open}
        onClose={handleClose}
        TransitionComponent={Transition}
      >
        <AppBar sx={{ position: 'relative' }}>
          <Toolbar>
            <IconButton
              edge="start"
              color="inherit"
              onClick={handleClose}
              aria-label="close"
            >
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
                    {Object.values(siteConfig).map(entry => <ConfigItem 
                                               name={entry.name}
                                               key={entry.name}
                                               datatype={entry.type}
                                               value={entry.value}
                                               configItemUpdated={entry.setter} />)}
                </List>
            </List>
          </ListItem>
          <Divider />
        </List>
      </Dialog>
    </div>
  );
});