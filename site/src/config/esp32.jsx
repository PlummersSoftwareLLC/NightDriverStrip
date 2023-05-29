const Esp32 = () => {
    const [config, setConfig] = useState();
    const [effects, setEffects ] = useState();
    const [service] = useState(eventManager());

    useEffect(() => {
        const chipRequest = (url,options,operation) => new Promise((resolve,reject) => {
            const aborter = new AbortController();
            const timer = setTimeout(() => aborter.abort(), 3000);
    
            return fetch(`${httpPrefix !== undefined ? httpPrefix : ""}${url}`,{...options, signal: aborter.signal })
                .then(resolve)
                .catch((err)=>{service.emit("Error",operation,err);reject(err);})
                .finally(()=>clearTimeout(timer));
        });

        chipRequest(`/settings`)
            .then(resp => resp.json())
            .then(setConfig)
            .catch(console.error);
        
        const subs = {
            subscribers: service.subscribe("subscription",sub=>{
                service.emit("ChipConfig",config,sub.eventId);
                service.emit("effectList",effects,sub.eventId);
            }),
            changeConfigSub : service.subscribe("SetChipConfig", newConfig => 
                chipRequest(`/settings`,{method: "POST", body:newConfig},"Set Chip Config")
                    .then(resp => resp.json())
                    .then(setConfig)),
            effectList: service.subscribe("refreshEffectList",()=> 
                chipRequest(`/effects`,{method:"GET"},"Get Effects")
                    .then(resp => resp.json())
                    .then(setEffects)),
            navigate: service.subscribe("navigate", (up)=> 
                chipRequest(`/${up ? "nextEffect" : "previousEffect"}`,{method:"POST"},"navigate")
                    .then(service.emit("refreshEffectList"))
                ),
        };

        service.subscribe("effectList", (effectList)=>{
            if (subs.navigateTo) {
                service.unsubscribe(subs.navigateTo);
                service.unsubscribe(subs.toggleEffect);
            }
            subs.navigateTo=service.subscribe("navigateTo", (effect)=>
                chipRequest(`/currentEffect`,
                    {method:"POST", body: new URLSearchParams({currentEffectIndex:effectList.Effects.findIndex(eff=>eff===effect)})},"navigateTo")
                    .then(service.emit("refreshEffectList"))),
            subs.toggleEffect=service.subscribe("toggleEffect", (effect) => 
                chipRequest(`/${effect.enabled?"disable":"enable"}Effect`,
                    {method:"POST", body:new URLSearchParams({effectIndex:effectList.Effects.findIndex(eff=>eff===effect)})},"effectEnable")
                    .then(service.emit("refreshEffectList")));

        });

        return () => {
            Object.values(subs).forEach(service.unsubscribe);
        };
    }, [service]);

    useEffect(() => {
        if (effects !== undefined){
            service.emit("effectList", effects);
        }}, ...(effects||{}));
    useEffect(() => {config !== undefined && service.emit("ChipConfig", config)}, ...(config||{}));

    return <div></div>;
}; 
