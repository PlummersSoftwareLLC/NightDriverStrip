import { useEffect, useMemo, useState } from "react";
import { Dialog, DialogActions, DialogContent, DialogTitle, Button, Box, TextField, Checkbox, FormControlLabel, GlobalStyles } from "@mui/material";
import {useTheme} from "@mui/material";
import httpPrefix from "../../../espaddr";
import PropTypes from "prop-types";
import parse from 'html-react-parser';
// Base styling for inputs.     
const textFieldProps = {
    margin: "dense",
    fullWidth: true,
    variant:"standard",
    FormHelperTextProps: { component: 'div' }
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

/**
 * Creates an Input field based on the settingType provided.
 * @param {Props} props React Props
 * @param {Object} props.setting Object defining the setting to load see. types.h/SettingSpec.
 * @param {Function} props.updateData The callback to provide the change value to the parent dialog.
 * @param {Function} props.updateError The callback to provide the error state to the parent dialog.
 * @returns {React.Component} The input field to load into the dialog
 */
const ConfigInput = ({setting, updateData, updateError}) => {
    const [value, setValue] = useState(setting.value);
    const [error, setError] = useState(false);
    const theme = useTheme();
    const jsxDescription = <>
        <GlobalStyles styles={
            { 
                "a:link": { color: theme.a.link },         
                "a:visited": { color: theme.a.visited }
            }
        } />
        {parse(setting.description)}
    </>;
    const [helper, setHelper] = useState(jsxDescription);
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
    const readOnly = !!setting.readOnly;
    switch(setting.type) {
    case settingType.Integer:
        return <TextField
            {...baseProps}
            {...textFieldProps}
            helperText={helper}
        />;  
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
                    setHelper(jsxDescription);
                }
                if(!v || isNaN(v)) {
                    setValue('');
                } else {
                    setValue(Math.floor(e.target.value));
                }
            }}
        />;
    case settingType.Float:
        return <TextField
            {...baseProps}
            {...textFieldProps}
            helperText={helper}
        />;
    case settingType.Boolean:
        return <FormControlLabel {...baseProps} control={
            <Checkbox 
                disabled={readOnly}
                defaultChecked={!!setting.value}               
                onChange={(e) => setValue(e.target.checked)}
            />
        }/>;
    case settingType.String: 
        return <TextField
            {...baseProps}
            {...textFieldProps}
            helperText={helper}
        />;
    case settingType.Palette:
        // FIXME Implement a Palette Config
        return <TextField
            {...baseProps}
            {...textFieldProps}
            helperText={helper}
        />;
    case settingType.Color:
        //  FIXME Implement a Color Config
        return <TextField
            {...baseProps}
            {...textFieldProps}
            helperText={helper}
        />;
    default:
        return <TextField
            {...baseProps}
            {...textFieldProps}
            helperText={helper}
        />;
    }
};

/**
 * Queries the device and loads the device settings, or effect settings for configuration.
 * @param {Props} props React Props.
 * @param {String} props.heading The heading to display of the dialog.
 * @param {Integer | undefined} props.effectIndex The index of the effect to load.
 *      if undefined load the device settings.
 * @param {boolean} props.open If the dialog should be open.
 * @param {Function} props.setOpen callback to close the dialog.
 * @returns 
 */
const ConfigDialog = ({heading, effectIndex, open, setOpen}) => {

    const settingsEndpoint = `${httpPrefix !== undefined ? httpPrefix : ""}/settings${effectIndex !== undefined ? '/effect': ''}`;
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
        await fetch(`${httpPrefix !== undefined ? httpPrefix : ""}/reset`, {method:"POST", body:new URLSearchParams({board: 1})});
        
    };
    
    const handleCancel = () => {
        setOpen(false);
    };

    
    useMemo(() => {
        const spec = `${settingsEndpoint}/specs${effectIndex !== undefined ? `?effectIndex=${effectIndex}` : ''}`;
        const current = `${settingsEndpoint}${effectIndex !== undefined ? `?effectIndex=${effectIndex}` : ''}`;
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
        <DialogTitle>{heading} Configuration</DialogTitle>
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

ConfigDialog.defaultProps = {
    effectIndex: undefined
};

ConfigDialog.propTypes = {
    heading: PropTypes.string.isRequired,
    effectIndex: PropTypes.number,
    open: PropTypes.bool.isRequired,
    setOpen: PropTypes.func.isRequired
};

const settingProps = PropTypes.shape({
    name: PropTypes.string.isRequired, 
    friendlyName: PropTypes.string.isRequired, 
    description: PropTypes.string.isRequired, 
    type: PropTypes.number.isRequired, 
    typeName: PropTypes.string.isRequired, 
    value: PropTypes.any,
    readOnly: PropTypes.bool,
    writeOnly: PropTypes.bool
});


ConfigInput.propTypes = {
    setting: settingProps,
    updateData: PropTypes.func,
    updateError: PropTypes.func
};

export default ConfigDialog;