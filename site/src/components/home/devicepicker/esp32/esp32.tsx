import { Icon, Typography } from "@mui/material";
import { useEffect, useState } from "react";
import { eventManager } from "../../../../services/eventManager/eventmanager";
import { safeJsonParse } from "../../../../utils/jsonparse";
import { INightDriverConfiguration, INightDriverConfigurationSpecs } from '../../../../models/config/nightdriver/nightdriver';
import { IEffects } from "../../../../models/config/nightdriver/effects";
import { IESPState } from "../../../../models/stats/espstate";
import { withStyles } from 'tss-react/mui';
import { esp32Style } from "./style"

interface IEsp32Props {
    activeHttpPrefix:string, 
    selected:boolean,
    classes?:any
}

export const Esp32 = withStyles(({activeHttpPrefix, selected, classes}:IEsp32Props) => {
    const [config, setConfig] = useState(undefined as unknown as INightDriverConfiguration);
    const [configSpec, setConfigSpec] = useState(undefined as unknown as INightDriverConfigurationSpecs[]);
    const [effects, setEffects ] = useState(undefined as unknown as IEffects);
    const [service] = useState(eventManager());

    const chipRequest = (url:string,options:RequestInit,operation:string):Promise<Response> =>
        new Promise<Response>((resolve,_reject) => {
            const aborter = new AbortController();
            const timer = setTimeout(() => aborter.abort(), 3000);

            fetch(`${activeHttpPrefix !== "Current Device" ? activeHttpPrefix : ""}${url}`,{...options, signal: aborter.signal })
                .then(resolve)
                .catch((err)=>service.emit("Error",{level:"error",type:options.method ?? "GET",target:operation,notification:err}))
                .finally(()=>clearTimeout(timer));
        });

    useEffect(() => {
        if (selected) {
            chipRequest(`/settings`,{method: "GET"},"Get Chip Setting")
                .then(resp => resp.text())
                .then(safeJsonParse<INightDriverConfiguration>())
                .then(cfg=>!cfg.hasError && setConfig({...cfg.parsed }))
                .then(()=>chipRequest(`/settings/specs`,{method:"GET"},"Get Chip Option Specs")
                    .then(resp => resp.text())
                    .then(safeJsonParse<INightDriverConfigurationSpecs[]>())
                    .then(cs=>!cs.hasError&&setConfigSpec([...cs.parsed])))
                .catch(console.error);
            let subs = {
                changeConfigSub : service.subscribe("SetChipConfig", (newConfig:INightDriverConfiguration) => {
                    const formData = new FormData();
                    Object.entries(newConfig).forEach(entry=>formData.append(entry[0],entry[1]));
                    chipRequest(`/settings`,{method: "POST", body:formData},"Set Chip Config")
                        .then(resp => resp.text())
                        .then(safeJsonParse<INightDriverConfiguration>())
                        .then(cfg=>!cfg.hasError&&setConfig({...cfg.parsed}))
                        .then(()=>service.emit("refreshEffectList"))
                        .catch(console.error)}),
                effectList: service.subscribe("refreshEffectList",()=> 
                    chipRequest(`/effects`,{method:"GET"},"Get Effects")
                    .then(resp => resp.text())
                    .then(safeJsonParse<IEffects>())
                    .then(effects=>!effects.hasError&&setEffects({...effects.parsed}))),
                statsRefresh: service.subscribe("refreshStatistics",() => 
                    chipRequest(`/statistics`,{method:"GET"},"Update Stats")
                        .then(resp => resp.text())
                        .then(safeJsonParse<IESPState>())
                        .then(stats=>!stats.hasError&&service.emit("statistics",stats.parsed))),
            };

            return () => Object.values(subs).forEach(service.unsubscribe);
        }
    }, [service,selected]);

    useEffect(() => {
        if (effects&&selected) {
            service.emit("effectList", effects);

            let subs = {
                subscribers: service.subscribe("subscription",sub=>{
                    service.emit("effectList",effects,sub.eventId);}),
                navigate: service.subscribe("navigate", (up)=> 
                    chipRequest(`/${up ? "nextEffect" : "previousEffect"}`,{method:"POST"},"navigate")
                        .then(()=>service.emit("refreshEffectList"))),
                navigateTo: service.subscribe("navigateTo", (index)=>
                    chipRequest(`/currentEffect`,
                        {method:"POST", body: new URLSearchParams({currentEffectIndex:index})},"navigateTo")
                        .then(()=>service.emit("refreshEffectList"))),
                toggleEffect: service.subscribe("toggleEffect", (effect) => 
                    chipRequest(`/${effect.enabled?"disable":"enable"}Effect`,
                        {method:"POST", body:new URLSearchParams({effectIndex:effects.Effects.findIndex(eff=>eff===effect).toString()})},"effectEnable")
                        .then(()=>service.emit("refreshEffectList"))),
            };
    
            return () => Object.values(subs).forEach(service.unsubscribe);
        }
    }, [effects, selected]);

    useEffect(() => {
        if (config&&selected) {
            service.emit("ChipConfig", config);
            const subscribers= service.subscribe("subscription",sub=>{service.emit("ChipConfig",config,sub.eventId)});
            return ()=>service.unsubscribe(subscribers);
        }
    }, [config,selected]);

    useEffect(() => {
        if(configSpec&&selected){
            service.emit("ChipConfigSpec", configSpec);
            const subscribers= service.subscribe("subscription",sub=>{service.emit("ChipConfigSpec",configSpec,sub.eventId)});
            return ()=>service.unsubscribe(subscribers);
        } 
    }, [configSpec,selected]);

    function getDeviceShortName():string {
        const ipAddrPattern = /(https?:\/\/)(([12]?\d{1,2}[.]){3}([12]?\d{1,2})).*/g
        if (activeHttpPrefix === "Current Device") {
            return "Esp32";
        } else {
            const res = ipAddrPattern.exec(activeHttpPrefix);
            return (res && (res[2] !== undefined)) ? res[2].toString():"";
        }
    }

    return <div className={classes.esp32}>
        {config ? <Icon className={classes.neticon}>settings_input_antenna</Icon> : <span/>}
        <Typography className={classes.name}>{getDeviceShortName()}</Typography>
    </div>;
},esp32Style); 
