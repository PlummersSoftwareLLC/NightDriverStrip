import { FormControl, Select, MenuItem } from "@mui/material";
import { useState, useEffect } from "react";
import { eventManager } from "../../../services/eventManager/eventmanager";
import { IEffects } from "../../../models/config/nightdriver/effects";
import { IESPState } from "../../../models/stats/espstate";
import { Esp32 } from "./esp32/esp32";

interface IDevicePickerProps {
    activeDevice:string, 
    setActiveDevice: (dev:string)=>void,
}

export function DevicePicker ({ activeDevice, setActiveDevice }:IDevicePickerProps) {
    const [service] = useState(eventManager());

    const [ netowrkDevices, _setNetworkDevices ] = useState([activeDevice]);
    const [ effects, setEffects ] = useState(undefined as unknown as IEffects);
    const [ statistics, setStatistics] = useState(undefined as unknown as IESPState);
    const [ _netMask ] = useState([192,168,0]);
    
    
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
            <Select
            value={activeDevice}
            onChange={event=>setActiveDevice(event.target.value)}>
                {netowrkDevices.map(networkDevice=><MenuItem key={networkDevice} value={networkDevice}>
                    <em><Esp32 activeHttpPrefix={activeDevice} selected={true} /></em>
                </MenuItem>)}
            </Select>
    </FormControl>;
};