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
        },
        UIMode: {
            name: "UI Mode",
            typeName: "string",
            value: "dark"
        }
    };

    const [config, setConfig] = useState(JSON.parse(window.sessionStorage.getItem("config")) || defaultConfig);
    const [effects, setEffects] = useState();
    const [effectList, setEffectList] = useState();
    const [effectSettings, setEffectSettings] = useState(JSON.parse(window.sessionStorage.getItem("effectSettings")) || {});
    const [service] = useState(eventManager());
    
    useEffect(() => {
        const subs = {
            setSiteConfigItem: service.subscribe("SetSiteConfigItem",({id, value})=>setConfig(prevConfig=>{
                if (prevConfig[id].value!==value){
                    prevConfig[id].value=value;
                    service.emit("SiteConfig",prevConfig);
                }
                return {...prevConfig};
            })),
            effectsSub:service.subscribe("effectList",effectList=>{setEffects({...effectList})}),
        }

        return ()=>Object.values(subs).forEach(service.unsubscribe);
    }, [service]);

    useEffect(()=>{
        effects && setEffectList(effects.Effects);
    },[effects]);

    useEffect(()=>{
        const getEffectName = (index) => {
            let dups = effectList.map((eff,idx)=>{return{idx,match:eff.name === effects.Effects[index].name}})
                                      .filter(matches=>matches.match);
            if (dups.length > 1) {
                return `${effectList[index].name}_${dups.findIndex(match=>match.idx === index)+1}`
            }
            return effectList[index].name;
        };

        const subs = {
            setEffectSettings: service.subscribe("setEffectSettings",({index,options})=>
                {setEffectSettings(prevEffectSettings=>{
                    return {...prevEffectSettings,[getEffectName(index)]:options};
                })}
            ,[effectSettings]),
        }

        return ()=>Object.values(subs).forEach(service.unsubscribe);

    },[effectList])

    useEffect(()=>{
        const subscription=service.subscribe("subscription",sub=>service.emit("SiteConfig",config,sub.eventId));
        service.emit("SiteConfig",config);
        window.sessionStorage.setItem("config",JSON.stringify(config));
        return ()=>service.unsubscribe(subscription);
    },[config])

    useEffect(()=>{
        const subscription=service.subscribe("subscription",sub=>service.emit("EffectSettings",effectSettings,sub.eventId));
        service.emit("EffectSettings",effectSettings);
        window.sessionStorage.setItem("effectSettings",JSON.stringify(effectSettings));
        return ()=>service.unsubscribe(subscription);
    },[effectSettings])

    return <div></div>;
};
