const DevicePicker = withStyles(devicePickerStyle)(props => {
    const [service] = useState(eventManager());

    const { classes, activeDevice, setActiveDevice } = props;
    const [ netowrkDevices, setNetworkDevices ] = useState([activeDevice]);
    const [ effects, setEffects ] = useState();
    const [ statistics, setStatistics] = useState(undefined);
    const [ netMask ] = useState([192,168,0]);
    
    
    useEffect(() => {
        const subs={
            effectsSub:service.subscribe("effectList",effectList=>{setEffects(effectList)}),
            statiscticsSub:service.subscribe("statistics",statistics=>{setStatistics(statistics)}),
        };
        return ()=>Object.values(subs).forEach(service.unsubscribe);
    }, [service]);

    function getHelperText() {
        if (effects) {
            return `${effects.Effects.length} effects - ${effects.Effects[effects.currentEffect].name}(${effects.currentEffect})`;
        }
        return "";
    }

    function getLabel() {
        if (statistics) {
            return `${Math.floor(statistics.CPU_USED)}% CPU ${Math.floor(100*((statistics.HEAP_SIZE - statistics.HEAP_FREE) / statistics.HEAP_SIZE))}% Heap`;
        }
        return "Active Device";
    }

    return <FormControl sx={{ m: 1, minWidth: 120 }}>
            <InputLabel className={classes.deviceLabel} id="device-label">{getLabel()}</InputLabel>
            <Select
            labelId="device-label"
            value={activeDevice}
            onChange={event=>setActiveDevice(event.target.value)}>
                {netowrkDevices.map(networkDevice=><MenuItem key={networkDevice} value={networkDevice}>
                    <em><Esp32 activeHttpPrefix={activeDevice} selected={true} /></em>
                </MenuItem>)}
            </Select>
            <FormHelperText>{getHelperText()}</FormHelperText>
    </FormControl>;
});