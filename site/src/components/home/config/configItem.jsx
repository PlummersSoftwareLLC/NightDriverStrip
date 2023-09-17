import { useState } from "react";
import { ListItem, FormControlLabel, Checkbox, ClickAwayListener, ListItemText, TextField } from "@mui/material";
import configStyle from "./style";

const ConfigItem = props => {
    const { name, value, configItemUpdated, datatype, classes } = props;
    const [ editing, setEditing] = useState(false);
    const [ configValue, setConfigValue] = useState(value);
    const getConfigValue = (value, type) => {
        switch (type) {
        case "int":
            return parseInt(value);
        case "float":
            return parseFloat(value);
        default:
            return value;
        }
    };
    
    if (datatype === "boolean") {
        return <ListItem button onClick={()=>!editing && setEditing(!editing)}>
            <FormControlLabel
                sx={configStyle.cblabel}
                label={name} 
                labelPlacement="top"
                control={<Checkbox 
                    defaultChecked={value}
                    onChange={event => {
                        setConfigValue(event.target.checked);
                        configItemUpdated(event.target.checked);
                    }} />} />
        </ListItem>;
    }

    return <ClickAwayListener onClickAway={()=>{configItemUpdated(configValue);setEditing(false);}}><ListItem button onClick={()=>!editing && setEditing(!editing)}>
        {!editing && <ListItemText sx={ configStyle.configDisplay }
            primary={name}
            secondary={configValue}/>}
        {editing && <TextField label={name} 
            variant="outlined"
            type={["int","float"].includes(datatype) ? "number" : "text"}
            pattern={datatype === "int" ? "^[0-9]+$" : (datatype === "float" ? "^[0-9]+[.0-9]*$" : ".*")}
            defaultValue={value}
            onChange={event => setConfigValue(getConfigValue(event.target.value,datatype)) } />}
    </ListItem></ClickAwayListener>;
};

export default ConfigItem;