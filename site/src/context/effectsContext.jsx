import { createContext, useEffect, useState } from "react";
import httpPrefix from "../espaddr";
import PropTypes from 'prop-types';

const EffectsContext = createContext(undefined);
const effectsEndpoint = `${httpPrefix !== undefined ? httpPrefix : ""}/effects`;
const refreshInterval = 30000; //30 Seconds

const EffectsProvider = ({ children }) => {
    const [effects, setEffects] = useState([]);
    const [activeInterval, setActiveInterval] = useState(2**32);
    const [pinnedEffect, setPinnedEffect] = useState(false);
    const [remainingInterval, setRemainingInterval] = useState(0);
    const [activeEffect, setActiveEffect] = useState(0);
    const [effectTrigger, setEffectTrigger] = useState(false);
    const [currentEffect, setCurrentEffect] = useState(Number(0));
    useEffect(() => {
        const getDataFromDevice = async (params) => {
            try {
                const {currentEffect, millisecondsRemaining, eternalInterval, effectInterval, Effects} = await fetch(effectsEndpoint, params).then(r => r.json());
                setActiveEffect(currentEffect);
                setRemainingInterval(millisecondsRemaining);
                setActiveInterval(effectInterval);  
                setEffects(Effects);
                setPinnedEffect(eternalInterval);
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
    
    useEffect(() => {
        setCurrentEffect(activeEffect);
        if (!pinnedEffect) {
            const timer = setTimeout(() => {
                // Timer expired, trigger a resync. 
                setEffectTrigger(s => !s);
            }, remainingInterval+10);
            return () => {
                clearTimeout(timer);
            };
        }
    }, [activeEffect, activeInterval])
    return (
        <EffectsContext.Provider value={{activeInterval, remainingInterval, activeEffect, pinnedEffect,currentEffect, effects, sync: () => setEffectTrigger(s => !s)}}>
            {children}
        </EffectsContext.Provider>
    );
};

EffectsProvider.propTypes = {
    children: PropTypes.oneOfType([
        PropTypes.arrayOf(PropTypes.node),
        PropTypes.node
    ]).isRequired
};

export {EffectsContext, EffectsProvider};