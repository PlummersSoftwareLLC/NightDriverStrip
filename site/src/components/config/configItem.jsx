const ConfigItem = withStyles(configStyle)(props => {
    const [ service ] = useState(eventManager());
  
    const { name, value, type, id } = props;
    const [ editing, setEditing] = useState(false);
    const [ configValue, setConfigValue] = useState(value);
    const getConfigValue = (value, dataType) => {
        switch (dataType) {
            case "int":
                return parseInt(value);
            case "float":
                return parseFloat(value);
            default:
                return value;
        }
    };

    useEffect(()=>{service.emit("SetSiteConfigItem",{value:configValue, id})},[configValue]);

    if (type === "boolean") {
        return <ListItem button onClick={_evt=>!editing && setEditing(!editing)}>
            <FormControlLabel
                label={name} 
                labelPlacement="top"
                control={<Checkbox 
                    defaultChecked={value}
                    onChange={event => setConfigValue(event.target.checked)}/>} />
        </ListItem>;
    }

    return <ClickAwayListener onClickAway={()=>{configItemUpdated(configValue);setEditing(false);}}><ListItem button onClick={_evt=>!editing && setEditing(!editing)}>
                {!editing && <ListItemText
                    primary={name}
                    secondary={configValue}/>}
                {editing && <TextField label={name} 
                                       variant="outlined"
                                       type={["int","float"].includes(type) ? "number" : "text"}
                                       pattern={type === "int" ? "^[0-9]+$" : (type === "float" ? "^[0-9]+[.0-9]*$" : ".*")}
                                       defaultValue={value}
                                       onChange={event => setConfigValue(getConfigValue(event.target.value,type)) } />}
            </ListItem></ClickAwayListener>;
});
