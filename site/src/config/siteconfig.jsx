const SiteConfig = () => {
    const [config, setConfig] = useState({});
    const [service] = useState(eventManager());

    useEffect(() => {
        const scfg = window.sessionStorage.getItem("config");
        let cfg = undefined;
        if (scfg === null) {
            cfg={
                statsRefreshRate: {
                    name: "Refresh rate",
                    type: "int",
                    value: 3
                },
                statsAnimateChange: {
                    name: "Animate chart",
                    type: "boolean",
                    value: false
                },
                maxSamples: {
                    name: "Chart points",
                    type: "int",
                    value: 50
                }};
        } else {
            cfg=JSON.parse(scfg);
        }
        setConfig(cfg);
        const subs = {
            subscription:service.subscribe("subscription",sub=>service.emit("SiteConfig",cfg,sub.eventId)),
            changeConfigSub: service.subscribe("SetSiteConfig", 
                (newConfig) => setConfig(prevConfig => {return {...prevConfig,...newConfig}})),
            setSiteConfigItem: service.subscribe("SetSiteConfigItem",({value, id})=>setConfig(prevConfig=>{
                prevConfig[id].value=value;
                return prevConfig;
            }))
        }

        return ()=>{Object.values(subs).forEach(service.unsubscribe)}
    }, [service]);

    useEffect(() => {
        if (config.effectInterval !== undefined) {
            window.sessionStorage.setItem("config",JSON.stringify(config));
            service.emit("SiteConfig", config);
        }
    }, [config]);

    return <div></div>;
};
