const Esp32 = () => {
    const [config, setConfig] = useState();
    const [configSpec, setConfigSpec] = useState();
    const [effects, setEffects ] = useState();
    const [service] = useState(eventManager());

    const chipRequest = (url,options,operation) => new Promise((resolve,_reject) => {
        const aborter = new AbortController();
        const timer = setTimeout(() => aborter.abort(), 3000);

        return fetch(`${httpPrefix !== undefined ? httpPrefix : ""}${url}`,{...options, signal: aborter.signal })
            .then(resolve)
            .catch((err)=>service.emit("Error",{level:"error",type:options.method || "GET",target:operation,notification:err}))
            .finally(()=>clearTimeout(timer));
    });

    useEffect(() => {
        chipRequest(`/settings`,{method: "GET"},"Get Chip Setting")
            .then(resp => resp.json())
            .then(cfg=>setConfig({...cfg}))
            .then(()=>chipRequest(`/settings/specs`,{method:"GET"},"Get Chip Option Specs")
                        .then(resp => resp.json())
                        .then(cs=>setConfigSpec([...cs])));
        
        let subs = {
            changeConfigSub : service.subscribe("SetChipConfig", newConfig => {
                const formData = new FormData();
                Object.entries(newConfig).forEach(entry=>formData.append(entry[0],entry[1]));
                chipRequest(`/settings`,{method: "POST", body:formData},"Set Chip Config")
                    .then(resp => resp.json())
                    .then(cfg=>setConfig({...cfg}))}),
            effectList: service.subscribe("refreshEffectList",()=> 
                chipRequest(`/effects`,{method:"GET"},"Get Effects")
                    .then(resp => resp.json())
                    .then(effects=>setEffects({...effects}))),
            statsRefresh: service.subscribe("refreshStatistics",() => 
                chipRequest(`/statistics`,{method:"GET"},"Update Stats")
                    .then(resp =>resp.json())
                    .then(stats=>service.emit("statistics",stats))),
        };

        return () => Object.values(subs).forEach(service.unsubscribe);
    }, [service]);

    useEffect(() => {
        if (effects) {
            service.emit("effectList", effects);

            let subs = {
                subscribers: service.subscribe("subscription",sub=>{
                    service.emit("effectList",effects,sub.eventId);}),
                navigate: service.subscribe("navigate", (up)=> 
                    chipRequest(`/${up ? "nextEffect" : "previousEffect"}`,{method:"POST"},"navigate")
                        .then(()=>service.emit("refreshEffectList"))),
                navigateTo: service.subscribe("navigateTo", (effect)=>
                    chipRequest(`/currentEffect`,
                        {method:"POST", body: new URLSearchParams({currentEffectIndex:effects.Effects.findIndex(eff=>eff===effect)})},"navigateTo")
                        .then(service.emit("refreshEffectList"))),
                toggleEffect: service.subscribe("toggleEffect", (effect) => 
                    chipRequest(`/${effect.enabled?"disable":"enable"}Effect`,
                        {method:"POST", body:new URLSearchParams({effectIndex:effects.Effects.findIndex(eff=>eff===effect)})},"effectEnable")
                        .then(service.emit("refreshEffectList"))),
            };
    
            return () => Object.values(subs).forEach(service.unsubscribe);
        }
    }, [effects]);

    useEffect(() => {
        if (config) {
            service.emit("ChipConfig", config);
            const subscribers= service.subscribe("subscription",sub=>{service.emit("ChipConfig",config,sub.eventId)});
            return ()=>service.unsubscribe(subscribers);
        }
    }, [config]);

    useEffect(() => {
        if(configSpec){
            service.emit("ChipConfigSpec", configSpec);
            const subscribers= service.subscribe("subscription",sub=>{service.emit("ChipConfigSpec",configSpec,sub.eventId)});
            return ()=>service.unsubscribe(subscribers);
        } 
    }, [configSpec]);

    return <div></div>;
}; 
