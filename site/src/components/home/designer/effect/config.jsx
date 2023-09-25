import { useEffect, useMemo, useState } from "react";
import { Dialog, DialogActions, DialogContent, DialogTitle, Button, Box, TextField, Checkbox, FormControlLabel } from "@mui/material";
import httpPrefix from "../../../../espaddr";
import PropTypes from "prop-types";


const settingsEndpoint = `${httpPrefix !== undefined ? httpPrefix : ""}/settings/effect`;    
const textFieldProps = {
    margin: "dense",
    fullWidth: true,
    variant:"standard"
};

// enum definition: see types.h/SettingSpec.SettingType
const settingType = {
    Integer: 0,
    PositiveBigInteger: 1,
    Float: 2,
    Boolean: 3,
    String: 4,
    Palette: 5,
    Color: 6
};

const ConfigInput = ({setting, updateData, updateError}) => {
    const [value, setValue] = useState(setting.value);
    const [error, setError] = useState(false);
    const [helper, setHelper] = useState(setting.description);
    const baseProps = {
        label: setting.friendlyName,
        id: setting.name,
        onChange: (e) => setValue(e.target.value),
        value: value
    };
    useEffect(() => {
        updateError(setting.name, error);
    }, [setting, updateError, error]);
    useEffect(() => {
        if(value !== setting.value) {
            if(value === '') {
                updateData(setting.name, undefined);
            } else {
                updateData(setting.name, value);
            }
        }
    }, [value, updateData, setting]);
    // TODO Add support for checking HasValidation and actual min max
    const readOnly = setting.description.includes("(read only)");
    switch(setting.type) {
    case settingType.Integer:     
    case settingType.PositiveBigInteger:

        return <TextField
            {...baseProps}
            {...textFieldProps}
            error={error}
            onError={(e) => updateError(setting.name, e)}
            helperText={helper}
            onChange={(e) => {
                const v = e.target.value;
                if( v === '' || v< 0 || v> 2**32) {
                    if(!error) {
                        setError(true);
                        setHelper(`Value must be between 0 and ${2**32}`);
                    }
                } else if(error){
                    setError(false);
                    setHelper(setting.description);
                }
                if(!v || isNaN(v)) {
                    setValue('');
                } else {
                    setValue(Math.floor(e.target.value));
                }
            }}
        />;
    case settingType.Float: 
    case settingType.Boolean:
        return <FormControlLabel {...baseProps} control={
            <Checkbox 
                disabled={readOnly}
                onChange={(e) => setValue(e.target.checked)}
            />
        }/>;
    case settingType.String: 
        return <TextField
            {...baseProps}
            {...textFieldProps}
            helperText={setting.description}
        />;
    case settingType.Palette:
        // TODO Implement a Palette Config
        return <TextField
            {...baseProps}
            {...textFieldProps}
            helperText={setting.description}
        />;
    case settingType.Color:
        // TODO Implement a Color Config
        return <TextField
            {...baseProps}
            {...textFieldProps}
            helperText={setting.description}
        />;
    default:
        return <TextField
            {...baseProps}
            {...textFieldProps}
            helperText={setting.description}
        />;
    }
};

const Config = ({effectName, effectIndex, open, setOpen}) => {
    const [content, setContent] = useState(<h2>Configuration Loading...</h2>);
    const [saveData, setSaveData] = useState({});
    const [errorFields, setErrorFields] = useState(new Set());
    const [disableSubmit, setDisableSubmit] = useState(false);

    const handleSubmit = async () => {
        await fetch(settingsEndpoint, {method:"POST", body:new URLSearchParams({effectIndex, ...saveData})}).then(r => r.json());
        setOpen(false);
    };
    const handleSubmitAndReboot = async () => {
        await handleSubmit();
        
    };
    
    const handleCancel = () => {
        setOpen(false);
    };

    
    useMemo(() => {
        const spec = `${settingsEndpoint}/specs?effectIndex=${effectIndex}`;
        const current = `${settingsEndpoint}?effectIndex=${effectIndex}`;
        const updateData = (name, value) => {
            setSaveData(d => {
                if(value !== undefined) {
                    d[name] = value;
                } else if(d[name])
                {
                    delete d[name];
                }
                return d;
            });
        };
    
        const updateError = (name, value) => { 
            if(value) {
                setErrorFields(e => {e.add(name); return e;});
            } else{
                setErrorFields(e => {e.delete(name); return e;});
            }   
            setDisableSubmit(errorFields.size > 0);
        };

        const fetchConfig = async () => {
            const [specData, curConfData] = await Promise.all([
                fetch(spec).then(r => r.json()),
                fetch(current).then(r => r.json())]); 
            
            setContent(<Box>
                {specData.map((setting) => {
                    if(setting.name in curConfData) {
                        setting.value = curConfData[setting.name];
                    }    
                    return <ConfigInput key={setting.name} setting={setting} updateData={updateData} updateError={updateError}></ConfigInput>;
                })}
            </Box>);
        };
        fetchConfig();
    // No need to re-render on error change as that's pushed from sub Components
    // eslint-disable-next-line react-hooks/exhaustive-deps
    }, [effectIndex]);

    return <Dialog open={open} onClose={handleCancel}>
        <DialogTitle>{effectName} Configuration</DialogTitle>
        <DialogContent>
            {content}
        </DialogContent>
        <DialogActions>
            <Button onClick={handleCancel}>Cancel</Button>
            <Button disabled={disableSubmit} onClick={handleSubmit}>Apply</Button>
            <Button disabled={disableSubmit} onClick={handleSubmitAndReboot}>Apply and Reboot Device</Button>
        </DialogActions>
    </Dialog>;
};

Config.propTypes = {
    effectName: PropTypes.string.isRequired,
    effectIndex: PropTypes.number.isRequired,
    open: PropTypes.bool.isRequired,
    setOpen: PropTypes.func.isRequired
};

const settingProps = PropTypes.shape({
    name: PropTypes.string.isRequired, 
    friendlyName: PropTypes.string.isRequired, 
    description: PropTypes.string.isRequired, 
    type: PropTypes.number.isRequired, 
    typeName: PropTypes.string.isRequired, 
    value: PropTypes.any
});

ConfigInput.propTypes = {
    setting: settingProps,
    updateData: PropTypes.func,
    updateError: PropTypes.func
};

export default Config;