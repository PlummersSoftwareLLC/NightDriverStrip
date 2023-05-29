const ConfigPanel = withStyles(configStyle)(props => {
  const [siteConfig, setSiteConfig] = useState();
  const [service] = useState(eventManager());

  useEffect(()=>{
    const sub = service.subscribe("SiteConfig",cfg=>setSiteConfig(cfg));
    return ()=>{service.unsubscribe(sub)};
  },[service]);

  if (siteConfig === undefined) {
    return <div>Loading...</div>
  }
  return (
      <List>
        {siteConfig?Object.entries(siteConfig).map(entry => <ConfigItem 
                                        key={entry[0]}
                                        id={entry[0]}
                                        {...entry[1]}/>):0}
      </List>
  );
});