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
            setSiteConfigItem: service.subscribe("SetSiteConfigItem",({id, value})=>setConfig(prevConfig=>{
                if (prevConfig[id].value!==value){
                    prevConfig[id].value=value;
                    window.sessionStorage.setItem("config",JSON.stringify(prevConfig));
                    service.emit("SiteConfig",prevConfig);
                }
                return {...prevConfig};
            }))
        }

        return ()=>Object.values(subs).forEach(service.unsubscribe);
    }, [service]);

    useEffect(()=>{
        const subscription=service.subscribe("subscription",sub=>service.emit("SiteConfig",config,sub.eventId));
        service.emit("SiteConfig",config);
        return ()=>service.unsubscribe(subscription);
    },[config])

    return <div></div>;
};
