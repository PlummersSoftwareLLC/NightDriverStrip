import { createContext, useEffect, useState } from "react";
import httpPrefix from "../espaddr";

const EffectsContext = createContext(undefined);
const effectsEndpoint = `${httpPrefix !== undefined ? httpPrefix : ""}/effects`;
const refreshInterval = 30000; //30 Seconds

const EffectsProvider = ({ children }) => {
    const [effects, setEffects] = useState([]);
    const [activeInterval, setActiveInterval] = useState(2**32);
    const [remainingInterval, setRemainingInterval] = useState(0);
    const [activeEffect, setActiveEffect] = useState(0);
    const [effectTrigger, setEffectTrigger] = useState(false);
    useEffect(() => {
        const getDataFromDevice = async (params) => {
            try {
                const {currentEffect, millisecondsRemaining, effectInterval, Effects} = await fetch(effectsEndpoint, params).then(r => r.json());
                setActiveEffect(currentEffect);
                setRemainingInterval(millisecondsRemaining);
                setActiveInterval(effectInterval);  
                setEffects(Effects);
            } catch(err) {
                console.debug("Aborted update");
            }
        };
        const timer = setInterval(() => {
            const controller = new AbortController();
            getDataFromDevice({signal: controller.signal});
        }, refreshInterval);
        getDataFromDevice();
        return () => {
            clearInterval(timer);
        };
    },[effectTrigger]);
    return (
        <EffectsContext.Provider value={{activeInterval, remainingInterval, activeEffect, effects, sync: () => setEffectTrigger(s => !s)}}>
            {children}
        </EffectsContext.Provider>
    );
};

export {EffectsContext as TimingContext, EffectsProvider as TimingProvider};