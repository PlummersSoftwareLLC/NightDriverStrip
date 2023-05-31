const SiteConfig = () => {
    const defaultConfig={
        statsRefreshRate: {
            name: "Refresh rate",
            typeName: "int",
            value: 3
        },
        statsAnimateChange: {
            name: "Animate chart",
            typeName: "boolean",
            value: false
        },
        maxSamples: {
            name: "Chart points",
            typeName: "int",
            value: 50
        }};

    const [config, setConfig] = useState(JSON.parse(window.sessionStorage.getItem("config")) || defaultConfig);
    const [service] = useState(eventManager());
    
    useEffect(() => {
        const subs = {
            subscription:service.subscribe("subscription",sub=>service.emit("SiteConfig",config,sub.eventId)),
            setSiteConfigItem: service.subscribe("SetSiteConfigItem",({id, value})=>setConfig(prevConfig=>{
                prevConfig[id].value=value;
                window.sessionStorage.setItem("config",JSON.stringify(prevConfig));
                service.emit("SiteConfig",prevConfig);
                return prevConfig;
            }))
        }

        return ()=>Object.values(subs).forEach(service.unsubscribe);
    }, [service]);

    return <div></div>;
};
