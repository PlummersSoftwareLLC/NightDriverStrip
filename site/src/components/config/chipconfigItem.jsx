const ChipConfigItem = withStyles(configStyle)(props => {
    const [ service ] = useState(eventManager());
  
    const { classes, name, value, typeName, friendlyName, id } = props;
    const [ editing, setEditing] = useState(false);
    const [ configValue, setConfigValue] = useState(value);
    const getConfigValue = (value, dataType) => {
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
        return <ListItem aria-label={friendlyName} className={classes.configitem} button onClick={_evt=>setEditing(false)}>
            <FormControlLabel
                sx={{ marginLeft: "0px" }}
                label={<Typography variant="tiny">{friendlyName}</Typography>} 
                labelPlacement="left"
                control={<Checkbox 
                    defaultChecked={value}
                    onChange={event => setConfigValue(event.target.checked)}/>} />
        </ListItem>;
    }

    return <ClickAwayListener onClickAway={()=>{setEditing(false)}}>
                <ListItem className={classes.configitem} button onClick={_evt=>!editing && setEditing(!editing)}>
                    {!editing && <ListItemText
                        primary={name}
                        secondary={value}/>}
                    {editing && <TextField label={friendlyName} 
                                        variant="outlined"
                                        type={["int","float","PositiveBigInteger"].includes(typeName) ? "number" : "text"}
                                        pattern={typeName.toLowerCase().indexOf("int") >= 0 ? "^[0-9]+$" : (typeName === "float" ? "^[0-9]+[.0-9]*$" : ".*")}
                                        defaultValue={value}
                                        onChange={event => setConfigValue(getConfigValue(event.target.value,typeName)) } />}
                </ListItem>
            </ClickAwayListener>;
});
