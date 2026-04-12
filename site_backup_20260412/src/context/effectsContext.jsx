import { createContext, useContext, useEffect, useRef, useState } from "react";
import { httpPrefix, wsPrefix } from "../espaddr";
import PropTypes from 'prop-types';
import { StatsContext } from "./statsContext";

const EffectsContext = createContext(undefined);
const restEffectsEndpoint = `${httpPrefix !== undefined ? httpPrefix : ""}/effects`;
const wsEffectsEndpoint = `${wsPrefix}/effects`;
const refreshInterval = 30000; //30 Seconds

const EffectsProvider = ({ children }) => {
    const [effects, setEffects] = useState([]);
    const [activeInterval, setActiveInterval] = useState(2 ** 32);
    const [pinnedEffect, setPinnedEffect] = useState(false);
    const [remainingInterval, setRemainingInterval] = useState(0);
    const [activeEffect, setActiveEffect] = useState(0);
    const [effectTrigger, setEffectTrigger] = useState(false);
    const [currentEffect, setCurrentEffect] = useState(Number(0));
    const [reconnectSocket, setReconnectSocket] = useState(false);

    const { effectsSocket } = useContext(StatsContext);
    const ws = useRef(null)

    const getDataFromDevice = async (params) => {
        try {
            const { currentEffect, millisecondsRemaining, eternalInterval, effectInterval, Effects } = await fetch(restEffectsEndpoint, params).then(r => r.json());
            setActiveEffect(currentEffect);
            setRemainingInterval(millisecondsRemaining);
            setActiveInterval(effectInterval);
            setEffects(Effects);
            setPinnedEffect(eternalInterval);
        } catch (err) {
            console.debug("Aborted update");
        }
    };

    // Start - http polling for initial load and IFF web sockets are not available.
    useEffect(() => {
        if (!effectsSocket) {
            const timer = setInterval(() => {
                const controller = new AbortController();
                getDataFromDevice({ signal: controller.signal });
            }, refreshInterval);
            getDataFromDevice();
            return () => {
                clearInterval(timer);
            };
        }
    }, [effectTrigger, effectsSocket]);

    useEffect(() => {
        if (!effectsSocket) {
            setCurrentEffect(activeEffect);
            if (!pinnedEffect) {
                const timer = setTimeout(() => {
                    // Timer expired, trigger a resync. 
                    setEffectTrigger(s => !s);
                }, remainingInterval + 10);
                return () => {
                    clearTimeout(timer);
                };
            }
        }
    }, [activeEffect, activeInterval, effectsSocket]);
    // End - http polling for initial load and IFF web sockets are not available.

    // Start - Logic to use Web socket if its avaiable after the initial load
    useEffect(() => {
        if (effectsSocket) {
            ws.current = new WebSocket(wsEffectsEndpoint);
            const wsCurrent = ws.current
            ws.current.ondisconnect = () => {
                setReconnectSocket(r => !r);
            }
            return () => {
                if (wsCurrent && wsCurrent.readyState === WebSocket.OPEN) {
                    wsCurrent.close();
                }
            };
        }
    }, [effectsSocket, reconnectSocket]);

    useEffect(() => {
        if (effectsSocket && ws.current !== null) {
            ws.current.onmessage = (event) => {
                const update = JSON.parse(event.data);
                if (update['currentEffectIndex'] !== undefined) {
                    // Current effect is actually an index in http endpoint. Ws is newer
                    // and therefore has a more accurate naming convention. 
                    setCurrentEffect(update['currentEffectIndex']);
                    setRemainingInterval(activeInterval);
                }
                if (update['interval'] !== undefined) {
                    const interval = update['interval'];
                    // Only one interval in the context of a ws push, therefore interval == activeInterval
                    setActiveInterval(interval);
                    setRemainingInterval(interval - remainingInterval);
                    setPinnedEffect(interval === 0);

                }
                if (update['effectListDirty']) {
                    getDataFromDevice();
                }
                if (update['effectsEnabledState']) {
                    update['effectsEnabledState'].forEach((change) => {
                        const index = change.index
                        delete change.index
                        setEffects(old => {
                            const cp = [...old];
                            cp[index] = { ...cp[index], ...change }
                            return cp;
                        })
                    });
                }
            }
        }
    }, [effectsSocket, reconnectSocket, activeInterval, remainingInterval, currentEffect, effects])

    // End - Logic to use Web socket if its avaiable after the initial load
    return (
        <EffectsContext.Provider value={{ activeInterval, remainingInterval, activeEffect, pinnedEffect, currentEffect, effects, sync: () => setEffectTrigger(s => !s) }}>
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

export { EffectsContext, EffectsProvider };
