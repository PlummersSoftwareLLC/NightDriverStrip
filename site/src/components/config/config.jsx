const ConfigPanel = withStyles(configStyle)(props => {
  const { classes } = props;
  const [siteConfig, setSiteConfig] = useState();
  const [chipConfig, setChipConfig] = useState();
  const [chipConfigSpec, setChipConfigSpec] = useState();
  const [service] = useState(eventManager());

  useEffect(()=>{
    const subs = {
      siteConfig: service.subscribe("SiteConfig",cfg=>{setSiteConfig(cfg)}),
      chipConfig: service.subscribe("ChipConfig",cfg=>{setChipConfig(cfg)}),
      chipConfigSpec: service.subscribe("ChipConfigSpec",cfg=>{setChipConfigSpec(cfg)})
    }
    return ()=>{Object.values(subs).forEach(service.unsubscribe)};
  },[service]);

  if (siteConfig === undefined) {
    return <div>Loading...</div>
  }

  return (
    <List className={classes.configsection}>
      <ListItem className={classes.configsection}>
        <ListItemText><Typography variant="overline" color="textSecondary">NightDriver</Typography></ListItemText>
        <Divider />
        {chipConfig&&chipConfigSpec?Object.entries(chipConfig).map(entry => <ChipConfigItem 
                                        key={entry[0]}
                                        id={entry[0]}
                                        value={entry[1]}
                                        {...chipConfigSpec.find(cs=>cs.name===entry[0])}/>):<div>Loading...</div>}
      </ListItem>
      <ListItem className={classes.configsection}>
        <ListItemText><Typography variant="overline" color="textSecondary">Site</Typography></ListItemText>
        <Divider />
        {siteConfig?Object.entries(siteConfig).map(entry => <SiteConfigItem 
                                        key={entry[0]}
                                        id={entry[0]}
                                        {...entry[1]}/>):<div>Loading...</div>}
      </ListItem>
    </List>
  );
});