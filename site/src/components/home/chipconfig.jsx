const ChipConfig = () => {
    const [config, setConfig] = useState();
    const [service] = useState(eventManager());

    useEffect(() => {
        const aborter = new AbortController();
        const timer = setTimeout(() => {
            setNextRefreshDate(Date.now());
        }, 3000);

        fetch(`${httpPrefix !== undefined ? httpPrefix : ""}/settings`, { signal: aborter.signal })
            .then(resp => resp.json())
            .then(cfg=>setConfig(cfg))
            .then(clearTimeout(timer))
            .catch(console.error);

        const sub=service.subscribe("subscription",sub=>config&&service.emit("ChipConfig",config,sub.eventId));
        const changeConfigSub = service.subscribe("SetChipConfig", updateConfig);

        return () => {
            aborter.abort();
            clearTimeout(timer);
            service.unsubscribe(sub);
            service.unsubscribe(changeConfigSub);
        };
    }, [service]);

    const updateConfig = (newConfig) => {
        const aborter = new AbortController();
        const timer = setTimeout(() => {
            setNextRefreshDate(Date.now());
        }, 3000);

        const data = new FormData();
        Object.entries(newConfig).forEach(entry => data.append(entry[0], entry[1]));

        fetch(`${httpPrefix !== undefined ? httpPrefix : ""}/settings`, { signal: aborter.signal, method: "POST", body: data })
            .then(resp => resp.json())
            .then(cfg => setConfig(cfg))
            .then(clearTimeout(timer))
            .catch(console.error);

        return () => {
            aborter.abort();
            clearTimeout(timer);
        };
    };

    useEffect(() => {
        if (config !== undefined) {
            service.emit("ChipConfig", config);
        }
    }, [config === undefined ? [] : [...config]]);

    return <div></div>;
};
