const ConfigPanel = withStyles(configStyle)(props => {
  const [ siteConfig, setSiteConfig ] = useState();
  const [ service ] = useState(eventManager());

  useEffect(()=>{
    const sub=service.subscribe("SiteConfig",(config)=>config && setSiteConfig(config));
    return ()=>service.unsubscribe(sub);
  },[service]);

  return (
    siteConfig ? 
      <List>
        {Object.entries(siteConfig).map(entry => <ConfigItem 
                                        name={entry[1].name}
                                        key={entry[1].name}
                                        datatype={entry[1].type}
                                        value={entry[1].value}
                                        configItemUpdated={value => {
                                          entry[1].value=value;
                                          service.emit("SetSiteConfig",{...siteConfig});
                                        }} 
                                        />)}
      </List>:
      <div>Loading...</div>
  );
});