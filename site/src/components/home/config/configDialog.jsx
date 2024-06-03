import { useEffect, useMemo, useState } from "react";
import { Dialog, DialogActions, DialogContent, DialogTitle, Button, Box, TextField, Checkbox, FormControlLabel, FormLabel, GlobalStyles, InputLabel, FormHelperText, IconButton, Icon, Stack, Slider, FormControl } from "@mui/material";
import {useTheme} from "@mui/material";
import httpPrefix from "../../../espaddr";
import PropTypes from "prop-types";
import parse from 'html-react-parser';
import { RgbColorPicker } from "react-colorful";

// Base styling for inputs.     
const textFieldProps = {
    fullWidth: true,
    variant:"standard",
    FormHelperTextProps: { component: 'div' }
};

// Base styling for form control
const formControlProps = {
    display: 'flex',
    paddingTop: '15px'
};

// enum definition: see types.h/SettingSpec.SettingType
const settingType = {
    Integer: 0,
    PositiveBigInteger: 1,
    Float: 2,
    Boolean: 3,
    String: 4,
    Palette: 5,
    Color: 6,
    Slider: 7
};

const intToRGB = (int) => {
    const bits = int.toString(2).padStart(24, 0);
    return {
        r: parseInt(bits.substring(0, 8), 2),
        g: parseInt(bits.substring(8, 16), 2),
        b: parseInt(bits.substring(16, 24), 2)
    }
}

const RGBToInt = ({r, g, b}) => {
    return parseInt(r.toString(2).padStart(8, 0) + g.toString(2).padStart(8, 0) + b.toString(2).padStart(8, 0), 2);
}

const ColorPickerDialog = ({title, initialColor, settingValue, setValue, open, closeFn}) => {
    const [color, setColor] = useState(initialColor)
            
    return <Dialog open={open} onClose={closeFn}>
        <DialogTitle>{title}</DialogTitle>
        <DialogContent sx={{maxWidth: '250px'}}>
            <RgbColorPicker color={color} onChange={(color) => setColor(color)}></RgbColorPicker>
            <Box sx={{minHeight: '15px'}}></Box>
            <Box>
                <TextField label="r" value={color.r} sx={{width: '33%'}}
                    onChange={(e) =>{
                        const input = e.target.value
                        let newR = Math.min(255, Math.max(0, parseInt(input)))
                        setColor( c => {
                            let {r, g,b} = c;
                            if (isNaN(newR)) {
                                r = input === '' ? 0 : r;  
                            } else {
                                r = newR
                            }
                            return {r, g, b}
                        })
                    }
                    }
                    InputProps={{ inputProps: { min: 0, max: 255 } }}
                />
                <TextField label="g" value={color.g} sx={{width: '33%'}} 
                    onChange={(e) =>{
                        const input = e.target.value
                        let newG = Math.min(255, Math.max(0, parseInt(input)))
                        setColor( c => {
                            let {r, g,b} = c;
                            if (isNaN(newG)) {
                                g = input === '' ? 0 : g;  
                            } else {
                                g = newG
                            }
                            return {r, g, b}
                        })
                    }}
                    InputProps={{ inputProps: { min: 0, max: 255 } }}
                />
                <TextField label="b" value={color.b} sx={{width: '33%'}}
                    onChange={(e) =>{
                        const input = e.target.value
                        let newB = Math.min(255, Math.max(0, parseInt(input)))
                        setColor( c => {
                            let {r, g, b} = c;
                            if (isNaN(newB)) {
                                b = input === '' ? 0 : b;  
                            } else {
                                b = newB
                            }
                            return {r, g, b}
                        })
                    }} 
                    InputProps={{ inputProps: { min: 0, max: 255 } }}
                />
            </Box>
        </DialogContent>
        <DialogActions>
            <Button onClick={ () => {setValue(RGBToInt(color)); closeFn();}}>Ok</Button>
            <Button onClick={() => setColor(intToRGB(settingValue))}>Reset</Button>
            <Button onClick={closeFn}>Cancel</Button>
        </DialogActions>
    </Dialog>
}

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
    const [additionalDialog, setAddionalDialog] = useState(-1)
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
        id: setting.name,
        onChange: (e) => setValue(e.target.value),
        value: value,
        error: error
    };
    useEffect(() => {
        updateError(setting.name, error);
    }, [setting, updateError, error]);
    useEffect(() => {
        if(value !== setting.value) {
            if(value === '') {
                if(setting.emptyAllowed === undefined) {
                    // No specification on emptyAllowed, default to not setting the value
                    updateData(setting.name, undefined);
                } else if (setting.emptyAllowed) {
                    // Empty value explicitly allowed. 
                    updateData(setting.name, value);
                } else {
                    // Empty value explicitly not allowed. Setting to the original value as a
                    // placeholder. Dialog cannot be saved in this state anyway. 
                    updateData(setting.name, setting.value);
                }
            } else {
                updateData(setting.name, value);
            }
        } else {
            updateData(setting.name, undefined)
        }
    }, [value, updateData, setting]);
    const readOnly = !!setting.readOnly;
    let min;
    let max;
    switch(setting.type) {
    case settingType.Integer:
        min = setting.minimumValue !== undefined ? setting.minimumValue : (-2)**31;
        max = setting.maximumValue !== undefined ? setting.maximumValue : 2**31 -1;
        return <FormControl sx={formControlProps}>
            <FormLabel>{setting.friendlyName}</FormLabel>
            <TextField
                {...baseProps}
                {...textFieldProps}
                onError={(e) => updateError(setting.name, e)}
                helperText={helper}
                onChange={(e) => {
                    numberChanged(e.target.value, error, jsxDescription, min, max, setError, setHelper, setValue, Math.floor);
                }}
            />
        </FormControl> 
    case settingType.PositiveBigInteger:
        min = setting.minimumValue !== undefined ? setting.minimumValue : 0;
        max = setting.maximumValue !== undefined ? setting.maximumValue : 2**32;
        return <FormControl sx={formControlProps}>
            <FormLabel>{setting.friendlyName}</FormLabel>
            <TextField
                {...baseProps}
                {...textFieldProps}
                error={error}
                onError={(e) => updateError(setting.name, e)}
                helperText={helper}
                onChange={(e) => {
                    numberChanged(e.target.value, error, jsxDescription, min, max, setError, setHelper, setValue, Math.floor);
                }}
            />
        </FormControl>
    case settingType.Float:
        min = setting.minimumValue !== undefined ? setting.minimumValue :  -3.4028235E+38;
        max = setting.maximumValue !== undefined ? setting.maximumValue : 3.4028235E+38;
        return <FormControl sx={formControlProps}>
            <FormLabel>{setting.friendlyName}</FormLabel>
            <TextField
                {...baseProps}
                {...textFieldProps}
                onError={(e) => updateError(setting.name, e)}
                helperText={helper}
                onChange={(e) => {
                    numberChanged(e.target.value, error, jsxDescription, min, max, setError, setHelper, setValue);
                }}
            />
        </FormControl> 
    case settingType.Boolean:
        return <FormControl sx={formControlProps}>
            <FormControlLabel 
                label={setting.friendlyName}
                control={
                    <Checkbox 
                        sx={{paddingBottom: '0px', paddingTop: '0px'}}
                        disabled={readOnly}
                        defaultChecked={!!setting.value}               
                        onChange={(e) => setValue(e.target.checked)}
                    />
                }
            />
            <FormHelperText sx={{marginLeft: '0px', marginTop: '0px'}}>{jsxDescription}</FormHelperText>
        </FormControl>
        
    case settingType.String: 
        return <FormControl sx={formControlProps}>
            <FormLabel>{setting.friendlyName}</FormLabel>
            <TextField
                {...baseProps}
                {...textFieldProps}
                helperText={helper}
                onChange={(e) => {
                    if (e.target.value === '' && (setting.emptyAllowed === false)) {
                    // Value is empty and empty is not allowed
                        setError(true);
                        setHelper(`Empty value is not allowed for this input. Original value was '${setting.value}'`);
                    } else {
                        setError(false);
                        setHelper(setting.description);
                    }
                    setValue(e.target.value)
                }}
            />
        </FormControl>
    case settingType.Palette:
        return <Box sx={{paddingTop: '10px'}}>
            <InputLabel sx={{ scale: "0.75" }}>{setting.friendlyName}</InputLabel>
            <Box flexDirection={"row"} display={"flex"} justifyContent={"space-between"} flexWrap={"wrap"}>
                {value.map((colorInt, index) => {
                    const color = intToRGB(colorInt);
                    return <Box key={index}> 
                        <Box sx={{ minWidth: '3vh', minHeight: '3vh', backgroundColor: `rgb(${color.r},${color.g},${color.b})`, border: `0.3vh solid`, borderRadius: '0.2vh' }} onClick={() => setAddionalDialog(index)}></Box>
                        {additionalDialog === index && 
                        <ColorPickerDialog title={setting.friendlyName} initialColor={color} settingValue={setting.value[index]} open={additionalDialog == index} closeFn={() => setAddionalDialog(-1)} 
                            setValue={(val) => setValue(v => v.map((old, i) => i === index ? val : old))}
                        />}
                    </Box>
                })}
            </Box>
            <FormHelperText>{jsxDescription}</FormHelperText>
        </Box> 
    case settingType.Color: {
        const color = intToRGB(value)
        
        return <Box sx={{paddingTop: '10px'}}>
            <InputLabel>{setting.friendlyName}</InputLabel>
            <Box flexDirection={"row"} display={"flex"}>
                <Box flexGrow={"1"} sx={{backgroundColor: `rgb(${color.r},${color.g},${color.b})`}} onClick={() => setAddionalDialog(0)}></Box>
                <IconButton onClick={() => setAddionalDialog(0)}><Icon>color_lens</Icon></IconButton>
            </Box>
            {additionalDialog == 0 && <ColorPickerDialog title={setting.friendlyName} initialColor={color} settingValue={setting.value} setValue={setValue} open={additionalDialog == 0} closeFn={() => setAddionalDialog(-1)} /> }
            <FormHelperText>{jsxDescription}</FormHelperText>
        </Box>
    }
    case settingType.Slider: 
        return <Box sx={{paddingTop: '10px'}}>
            <InputLabel>{setting.friendlyName}</InputLabel>
            <Stack>
                <Slider min={setting.minimumValue} max={setting.maximumValue} valueLabelDisplay="auto" value={value} onChange={(e) => setValue(e.target.value)}> </Slider>
            </Stack>
            <FormHelperText>{jsxDescription}</FormHelperText>
        </Box>
    default:
        return <FormControl sx={formControlProps}>
            <FormLabel>{setting.friendlyName}</FormLabel>
            <TextField
                {...baseProps}
                {...textFieldProps}
                helperText={helper}
            />
        </FormControl>
    }
};

/**
 * 
 * @param {Number} value The value configured by the user
 * @param {Boolean} error If the field is currently in an error state
 * @param {String} description The description to show when in an ok state
 * @param {Number} min The minimum value for the number determined by type or config
 * @param {Number} max The maximum value for the number determined by type or config
 * @param {Function(Boolean)} setError A callback to set the field to an error state 
 * @param {Function(String)} setHelper A callback to set the helper text  
 * @param {Function(Number)} setValue A callback to set the internal state tracking the variable
 * @param {Function(Number)} filterFn A filter to apply when saving the number E.g. Math.floor for an Integer
 */
const numberChanged = (value, error, description, min, max, setError, setHelper, setValue, filterFn = (e) => e) => {
    if( !value || isNaN(value) || value === '' || value < min || value > max) {
        if(!error) {
            setError(true);
            setHelper(`Value must be between ${min.toLocaleString()} and ${max.toLocaleString()}`);
        }
    } else if(error){
        setError(false);
        setHelper(description);
    }
    if(!value || isNaN(value)) {
        setValue('');
    } else {
        setValue(filterFn(value));
    }
}

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
const ConfigDialog = ({heading, effectIndex, open, setOpen, saveCallback}) => {

    const settingsEndpoint = `${httpPrefix !== undefined ? httpPrefix : ""}/settings${effectIndex !== undefined ? '/effect': ''}`;
    const [content, setContent] = useState(<h2>Configuration Loading...</h2>);
    const [saveData, setSaveData] = useState({});
    const [errorFields, setErrorFields] = useState(new Set());
    const [disableSubmit, setDisableSubmit] = useState(false);

    const handleSubmit = async () => {
        await fetch(settingsEndpoint, {method:"POST", body:new URLSearchParams({effectIndex, ...saveData})}).then(r => r.json());
        if(saveCallback !== undefined) {
            saveCallback();
        }
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
    effectIndex: undefined,
    saveCallback: undefined
};

ConfigDialog.propTypes = {
    heading: PropTypes.string.isRequired,
    effectIndex: PropTypes.number,
    open: PropTypes.bool.isRequired,
    setOpen: PropTypes.func.isRequired,
    saveCallback: PropTypes.func
};

const settingProps = PropTypes.shape({
    name: PropTypes.string.isRequired, 
    friendlyName: PropTypes.string.isRequired, 
    description: PropTypes.string.isRequired, 
    type: PropTypes.number.isRequired, 
    typeName: PropTypes.string.isRequired, 
    value: PropTypes.any,
    readOnly: PropTypes.bool,
    writeOnly: PropTypes.bool,
    emptyAllowed: PropTypes.bool,
    minimumValue: PropTypes.number,
    maximumValue: PropTypes.number
});


ConfigInput.propTypes = {
    setting: settingProps,
    updateData: PropTypes.func,
    updateError: PropTypes.func
};

ColorPickerDialog.propTypes = {
    title: PropTypes.string.isRequired, 
    initialColor: PropTypes.shape({
        r: PropTypes.number.isRequired,
        g: PropTypes.number.isRequired,
        b: PropTypes.number.isRequired,
    }).isRequired, 
    settingValue: PropTypes.number.isRequired, 
    setValue: PropTypes.func.isRequired, 
    open: PropTypes.bool.isRequired, 
    closeFn: PropTypes.func.isRequired
}

export default ConfigDialog;