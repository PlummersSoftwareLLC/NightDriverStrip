import { Dialog, DialogActions, DialogContent, DialogTitle, Button, Box, TextField, Checkbox, FormControlLabel } from "@mui/material";
import PropTypes from "prop-types"
import httpPrefix from "../../../../espaddr";
import { useEffect, useState } from "react";

const textFieldProps = {
    margin: "dense",
    fullWidth: true,
    variant:"standard"
}

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

const ConfigInput = ({setting}) => {
    const [value, setValue] = useState(setting.value)
    const baseProps = {
        label: setting.friendlyName,
        id: setting.name,
        onChange: (e) => setValue(e.target.value),
        value: value
        

    }
    // TODO Add support for checking HasValidation and actual min max
    switch(setting.type) {
    case settingType.Integer:     
    case settingType.PositiveBigInteger:     
        return <TextField
            {...baseProps}
            {...textFieldProps}
            error={value < 0 || value > 2**32}
            helperText={`${setting.description} Value must be between 0 and ${2**32}`}
            onChange={(e) => {
                const v = e.target.value
                if(!v) {
                    // TODO handle value not set on save
                    setValue(undefined)
                }else if(isNaN(v)) {
                    setValue(0)
                } else {
                    setValue(Math.floor(e.target.value))
                }
            }}
        />
    case settingType.Float: 
    case settingType.Boolean:
        const readOnly = setting.description.includes("(read only)")
        return <FormControlLabel {...baseProps} control={<Checkbox checked={value || value==="true"} disabled={readOnly}></Checkbox>}/>
    case settingType.String: 
        return <TextField
            {...baseProps}
            {...textFieldProps}
            helperText={setting.description}
        />
    case settingType.Palette:
        // TODO Implement a Palette Config
        return <TextField
            {...baseProps}
            {...textFieldProps}
            helperText={setting.description}
        />
    case settingType.Color:
        // TODO Implement a Color Config
        return <TextField
            {...baseProps}
            {...textFieldProps}
            helperText={setting.description}
        />
    default:
        return <TextField
            {...baseProps}
            {...textFieldProps}
            helperText={setting.description}
        />
    }
}

const Config = ({effectName, effectIndex, open, setOpen}) => {
    const [content, setContent] = useState(<h2>Configuration Loading...</h2>)
    const handleSubmit = () => {
        setOpen(false)
    }
    
    const handleCancel = () => {
        setOpen(false)
    }
    
    useEffect(() => {
        const settingsEndpoint = `${httpPrefix !== undefined ? httpPrefix : ""}/settings/effect`
        const spec = `${settingsEndpoint}/specs?effectIndex=${effectIndex}`
        const current = `${settingsEndpoint}?effectIndex=${effectIndex}`
        const fetchConfig = async () => {
            const [specData, curConfData] = await Promise.all([
                fetch(spec).then(r => r.json()),
                fetch(current).then(r => r.json())]) 
            
            setContent(<Box>
                {specData.map((setting) => {
                    if(setting.name in curConfData) {
                        setting.value = curConfData[setting.name]
                    }    
                    return <ConfigInput key={setting.name} setting={setting}></ConfigInput>
                })}
            </Box>)
  

        }
        if(open) {
            fetchConfig()
        }
    }, [open, effectIndex])
    return <Dialog open={open} onClose={handleCancel}>
        <DialogTitle>{effectName} Configuration</DialogTitle>
        <DialogContent>
            {content}
        </DialogContent>
        <DialogActions>
            <Button onClick={handleCancel}>Cancel</Button>
            <Button onClick={handleSubmit}>Submit</Button>
        </DialogActions>
    </Dialog>
}

Config.propTypes = {
    effectName: PropTypes.string.isRequired,
    effectIndex: PropTypes.number.isRequired,
    open: PropTypes.bool.isRequired,
    setOpen: PropTypes.func.isRequired
}

const settingProps = PropTypes.shape({
    name: PropTypes.string.isRequired, 
    friendlyName: PropTypes.string.isRequired, 
    description: PropTypes.string.isRequired, 
    type: PropTypes.number.isRequired, 
    typeName: PropTypes.string.isRequired, 
    value: PropTypes.any
})

ConfigInput.propTypes = {
    setting: settingProps
}

export default Config;