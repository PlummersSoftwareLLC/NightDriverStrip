const SiteConfig = () => {
    const [config, setConfig] = useState();
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
        service.subscribe("subscription",sub=>service.emit("SiteConfig",cfg,sub.eventId));
    }, [service]);

    const changeConfigSub = service.subscribe("SetSiteConfig", (newConfig) => {
        setConfig({...newConfig});
        return service.unsubscribe(changeConfigSub);
    });

    useEffect(() => {
        if (config !== undefined) {
            window.sessionStorage.setItem("config",JSON.stringify(config));
            service.emit("SiteConfig", config);
        }
    }, [config]);

    return <div></div>;
};
