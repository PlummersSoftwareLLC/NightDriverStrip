import { ListItem, FormControlLabel, Typography, Checkbox, ClickAwayListener, ListItemText, TextField, ListItemButton } from "@mui/material";
import { useState, useEffect } from "react";
import { eventManager } from "../../services/eventManager/eventmanager";

interface IChipConfigItemProps { 
    name: string;
    value: string|number|boolean;
    typeName: string;
    friendlyName: string;
    id: string }

export function ChipConfigItem({ name, value, typeName, friendlyName, id }:IChipConfigItemProps) {
    const [ service ] = useState(eventManager());
  
    const [ editing, setEditing] = useState(false);
    const [ configValue, setConfigValue] = useState(value);
    const getConfigValue = (value, dataType):number|string|boolean => {
        switch (dataType) {
            case "PositiveBigInteger":
            case "int":
                return parseInt(value);
            case "float":
                return parseFloat(value);
            default:
                return value;
        }
    };
    useEffect(()=>{!editing && 
                    value !== getConfigValue(configValue,typeName) &&
                    service.emit("SetChipConfig",{[id]:getConfigValue(configValue,typeName)})},[configValue,editing]);

    if (typeName.toLowerCase() === "boolean") {
        return <ListItemButton aria-label={friendlyName} onClick={_evt=>setEditing(false)}>
            <FormControlLabel
                sx={{ marginLeft: "0px" }}
                label={<Typography variant="caption">{friendlyName}</Typography>} 
                labelPlacement="start"
                control={<Checkbox 
                    defaultChecked={value as boolean}
                    onChange={event => setConfigValue(event.target.checked)}/>} />
        </ListItemButton>;
    }

    return <ClickAwayListener onClickAway={()=>{setEditing(false)}}>
                <ListItemButton onClick={_evt=>!editing && setEditing(!editing)}>
                    {!editing && <ListItemText
                        primary={name}
                        secondary={value}/>}
                    {editing && <TextField label={friendlyName} 
                                        variant="outlined"
                                        type={["int","float","PositiveBigInteger"].includes(typeName) ? "number" : "text"}
                                        defaultValue={value}
                                        onChange={event => setConfigValue(getConfigValue(event.target.value,typeName)) } />}
                </ListItemButton>
            </ClickAwayListener>;
}
