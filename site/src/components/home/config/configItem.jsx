const ConfigItem = withStyles(configStyle)(props => {
    const { name, value, configItemUpdated, datatype, classes } = props;
    const [ editing, setEditing] = useState(false);
    const [ configValue, setConfigValue] = useState(value);
    const getConfigValue = (value, type) => {
        switch (type) {
            case "int":
                return parseInt(value)
            case "float":
                return parseFloat(value)
            default:
                return value;
        }
    }
    const theme = useTheme();

    if (datatype === "boolean") {
        return <ListItem button onClick={_evt=>!editing && setEditing(!editing)}>
            <FormControlLabel
                className={classes.cblabel}
                label={name} 
                labelPlacement="start"
                control={<Checkbox 
                    defaultChecked={value}
                    onChange={event => {
                        setConfigValue(event.target.checked);
                        configItemUpdated(event.target.checked);
                    }} />} />
        </ListItem>;
    }

    return <ListItem button onClick={_evt=>!editing && setEditing(!editing)}>
                {!editing && <ListItemText className={ classes.configDisplay }
                    primary={name}
                    secondary={configValue}/>}
                {editing && <TextField label={name} 
                                       variant="outlined"
                                       type={["int","float"].includes(datatype) ? "number" : "text"}
                                       pattern={datatype === "int" ? "^[0-9]+$" : (datatype === "float" ? "^[0-9]+[.0-9]*$" : ".*")}
                                       defaultValue={value}
                                       onChange={event => setConfigValue(getConfigValue(event.target.value,datatype)) } />}
                    <Box className={classes.saveIcons}>
                        {editing && <IconButton color="primary" 
                                            aria-label="Save" 
                                            component="label"
                                            onClick={_evt => {
                                                configItemUpdated(configValue)
                                                setEditing(false);
                                            }}>
                                    <Icon>save</Icon>
                                </IconButton>}
                        {editing && <IconButton color="secondary" 
                                                aria-label="Cancel" 
                                                component="label"
                                                onClick={_evt => {
                                                    setConfigValue(value);
                                                    setEditing(false);
                                                }}>
                                        <Icon>cancel</Icon>
                                    </IconButton>}
                    </Box>
            </ListItem>;
});
