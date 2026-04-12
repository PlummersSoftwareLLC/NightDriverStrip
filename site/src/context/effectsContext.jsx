import { createContext, useCallback, useContext, useEffect, useRef, useState } from "react";
import { httpPrefix, wsPrefix } from "../espaddr";
import { StatsContext } from "./statsContext";

const EffectsContext = createContext(undefined);
const restEndpoint = `${httpPrefix !== undefined ? httpPrefix : ""}/effects`;
const wsEndpoint   = `${wsPrefix}/effects`;
const POLL_MS      = 30000;

const EffectsProvider = ({ children }) => {
    const [effects,           setEffects]           = useState([]);
    const [activeInterval,    setActiveInterval]    = useState(2 ** 32);
    const [pinnedEffect,      setPinnedEffect]      = useState(false);
    const [remainingInterval, setRemainingInterval] = useState(0);
    const [currentEffect,     setCurrentEffect]     = useState(0);

    const { effectsSocket } = useContext(StatsContext);
    const ws = useRef(null);

    // Stable fetch function — no dependencies, never gets aborted by state changes
    const fetchEffects = useCallback(async (signal) => {
        try {
            const data = await fetch(restEndpoint, signal ? { signal } : {}).then(r => r.json());
            setCurrentEffect(data.currentEffect);
            setRemainingInterval(data.millisecondsRemaining ?? 0);
            setActiveInterval(data.effectInterval ?? 2 ** 32);
            setEffects(data.Effects ?? []);
            setPinnedEffect(data.eternalInterval ?? false);
        } catch (_) {}
    }, []);

    // Always fetch effects on mount to populate the initial list
    useEffect(() => {
        fetchEffects();
    }, [fetchEffects]);

    // HTTP polling when not using WebSocket (30s interval, no immediate re-fetch on mount)
    useEffect(() => {
        if (effectsSocket) return;
        const ctrl = new AbortController();
        const id = setInterval(() => fetchEffects(ctrl.signal), POLL_MS);
        return () => { clearInterval(id); ctrl.abort(); };
    }, [effectsSocket, fetchEffects]);

    // Auto-advance: trigger a re-fetch when the current effect should have rotated
    useEffect(() => {
        if (effectsSocket || pinnedEffect || !remainingInterval) return;
        const id = setTimeout(() => fetchEffects(), remainingInterval + 250);
        return () => clearTimeout(id);
    }, [currentEffect, activeInterval, remainingInterval, effectsSocket, pinnedEffect, fetchEffects]);

    // WebSocket for live effect updates
    useEffect(() => {
        if (!effectsSocket) return;
        let reconnectTimer = null;
        let intentionalClose = false;

        const connect = () => {
            const sock = new WebSocket(wsEndpoint);
            ws.current = sock;

            sock.onopen = () => {
                // Refresh full list when WebSocket connects (or reconnects)
                fetchEffects();
            };

            sock.onmessage = (event) => {
                try {
                    const u = JSON.parse(event.data);
                    if (u.currentEffectIndex !== undefined) setCurrentEffect(u.currentEffectIndex);
                    if (u.interval !== undefined) {
                        setActiveInterval(u.interval);
                        setPinnedEffect(u.interval === 0);
                    }
                    if (u.effectListDirty) fetchEffects();
                    if (u.effectsEnabledState) {
                        u.effectsEnabledState.forEach(({ index, ...patch }) => {
                            setEffects(old => {
                                const cp = [...old];
                                if (cp[index]) cp[index] = { ...cp[index], ...patch };
                                return cp;
                            });
                        });
                    }
                } catch (_) {}
            };

            sock.onclose = () => {
                if (!intentionalClose) {
                    reconnectTimer = setTimeout(connect, 3000);
                }
            };

            sock.onerror = () => sock.close();
        };

        connect();

        return () => {
            intentionalClose = true;
            clearTimeout(reconnectTimer);
            if (ws.current && ws.current.readyState !== WebSocket.CLOSED) {
                ws.current.close();
            }
        };
    }, [effectsSocket, fetchEffects]);

    return (
        <EffectsContext.Provider value={{
            activeInterval, remainingInterval, pinnedEffect,
            currentEffect, effects,
            sync: fetchEffects,
        }}>
            {children}
        </EffectsContext.Provider>
    );
};

export { EffectsContext, EffectsProvider };
