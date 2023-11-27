import {useState, useContext, useEffect} from 'react';
import {IconButton, Icon, Typography, Box, Link, ClickAwayListener, TextField, Checkbox, FormControlLabel, InputAdornment} from '@mui/material';
import Countdown from './countdown/countdown';
import Effect from './effect/effect';
import designStyle from './style';
import httpPrefix from '../../../espaddr';
import PropTypes from 'prop-types';
import { EffectsContext} from '../../../context/effectsContext';
import { msToTimeDisp } from '../../../util/time';

const moveEffectEndpoint = `${httpPrefix !== undefined ? httpPrefix : ""}/moveEffect`;
const DesignerPanel = ({ open, addNotification }) => {
    const config = JSON.parse(localStorage.getItem('designerConfig'));
    const {pinnedEffect, activeInterval, sync, effects} = useContext(EffectsContext);
    const activeItervalDisp = Math.floor(activeInterval / 1000)
    const [ editing, setEditing ] = useState(false);
    const [ requestRunning, setRequestRunning ] = useState(false);
    const [ pendingInterval, setPendingInterval ] = useState(activeItervalDisp);
    const [gridLayout, setGridlayout] = useState(config && config.gridLayout !== undefined ? config.gridLayout : true)
    const [dragging, setDragging] = useState(undefined)
    const [dropTarget, setDropTarget] = useState(undefined)
    const [showDisabled, setShowDisabled] =  useState(config && config.showDisabled !== undefined ? config.showDisabled : true)

    // save users state to storage so the page reloads where they left off. 
    useEffect(() => {
        localStorage.setItem('designerConfig', JSON.stringify({
            gridLayout,
            showDisabled
        }));
    }, [gridLayout, showDisabled]);

    useEffect(()=> {
        setPendingInterval(Math.floor(activeInterval / 1000))
    }, [activeInterval])

    const chipRequest = (url,options,operation) => {
        return fetch(url,options).catch(err => {addNotification("Error",operation,err); throw err});
    }

    const navigateTo = (idx)=>{
        setRequestRunning(true);
        return chipRequest(`${httpPrefix !== undefined ? httpPrefix : ""}/currentEffect`,{method:"POST", body: new URLSearchParams({currentEffectIndex:idx})}, "navigateTo")
            .then(sync)
            .finally(()=>setRequestRunning(false));
        
    };

    const effectEnable = (idx,enable)=>{
        setRequestRunning(true);
        return chipRequest(`${httpPrefix !== undefined ? httpPrefix : ""}/${enable?"enable":"disable"}Effect`,{method:"POST", body:new URLSearchParams({effectIndex:idx})},"effectEnable")
            .then(sync)
            .finally(()=>setRequestRunning(false));
    };

    const navigate = (up)=>{
        setRequestRunning(true);
        return chipRequest(`${httpPrefix !== undefined ? httpPrefix : ""}/${up ? "nextEffect" : "previousEffect"}`,{method:"POST"},"nvigate")
            .then(sync)
            .finally(()=>setRequestRunning(false));
    };

    const updateEventInterval = (interval)=>{
        setEditing(false);
        setRequestRunning(true);
        return chipRequest(`${httpPrefix !== undefined ? httpPrefix : ""}/settings`,
            {
                method:"POST",
                body: new URLSearchParams({effectInterval:interval * 1000})
            },"updateEventInterval")
            .then(sync)
            .finally(()=>setRequestRunning(false));
        
    };

    const displayHeader = ()=>{
        return <Box sx={designStyle.effectsHeaderValue}>
            <Typography variant="little" color="textPrimary">Interval</Typography>:
            <Link href="#" variant="little" color="textSecondary" sx={pinnedEffect ? {height: "25px"}: {}} onClick={() => setEditing(true)}>{pinnedEffect ?<Icon>all_inclusive</Icon> : msToTimeDisp(activeInterval)}</Link>
        </Box>;
    };

    const editingHeader = ()=>{
        return <ClickAwayListener onClickAway={()=>{updateEventInterval(pendingInterval)}}>
            <Box sx={designStyle.effectsHeaderValue}>
                <TextField label="Interval"
                    variant="outlined"
                    defaultValue={activeItervalDisp}
                    value={pendingInterval}
                    InputProps={{
                        endAdornment: <InputAdornment position='end'>sec</InputAdornment>,
                    }}
                    onKeyUp={e => {
                        if(e.key === "Enter") {
                            updateEventInterval(pendingInterval)
                        }
                    }}
                    onChange={e => {
                        const onlyNums = e.target.value.replace(/[^0-9]/i, '');
                        setPendingInterval(onlyNums);
                    }} />
            </Box></ClickAwayListener>;
    };

    if (!effects && open){
        return <Box>Loading....</Box>;
    }
    const hiddenClasses = !open && designStyle.hidden;
    return effects && <Box sx={{...designStyle.root, ...hiddenClasses}}>
        <Box sx={designStyle.effectsHeader}>
            {editing ?
                editingHeader():
                displayHeader()}
            <Countdown
                label="Time Remaining"/>
            {(effects.length > 1) && <Box>
                <IconButton disabled={requestRunning} onClick={()=>navigate(false)}><Icon>skip_previous</Icon></IconButton>
                <IconButton disabled={requestRunning} onClick={()=>navigate(true)}><Icon>skip_next</Icon></IconButton>
                <IconButton disabled={requestRunning} onClick={()=>sync()}><Icon>refresh</Icon></IconButton>
            </Box>}
            <Box sx={{flexGrow: '1'}}></Box>
            <FormControlLabel control={<Checkbox checked={showDisabled} onClick={() => setShowDisabled(state => !state)}/>} label="Show Disabled"/>
            <Box sx={{justifySelf: 'flex-end'}}><IconButton onClick={() => setGridlayout(prev => !prev)}><Icon>{gridLayout ? 'list_icon' : 'grid_view'}</Icon></IconButton></Box>
        </Box>
        <Box sx={gridLayout? designStyle.gridEffects : designStyle.listEffects}
            onDragOver={(event) => handleDragOver(event, undefined, setDropTarget)} 
            onDrop={(event) => {
                event.preventDefault();
                if(dragging !== undefined && dropTarget !== undefined && dragging !== dropTarget) {
                    return fetch(moveEffectEndpoint, {method:"POST", body:new URLSearchParams({effectIndex: dragging, newIndex: dropTarget})}).then(() => {
                        setDragging(undefined);
                        setDropTarget(undefined);
                        sync();
                    });
                }}}>            
            {effects.map((effect,idx) => (effect.enabled || showDisabled) && <Effect
                onDragStart={(event, index) => {
                    handleDragStart(event, index, setDragging)
                }} 
                onDragOver={(event, index) => {handleDragOver(event, index, setDropTarget)}}
                key={`effect-${idx}`}
                effect={effect}
                effectIndex={idx}
                navigateTo={navigateTo}
                requestRunning={requestRunning}
                effectEnable={effectEnable}
                gridLayout={gridLayout}/>
            )}
        </Box>
    </Box>;
};

function handleDragStart(event, index, setDragging) { 
    setDragging(index)
    event.dataTransfer.setData("index", index)
}

function handleDragOver(event, index, setDropTarget) {
    event.preventDefault();
    if (index !== undefined) {
        setDropTarget(index)
    }
}

DesignerPanel.propTypes = {
    open: PropTypes.bool.isRequired, 
    addNotification: PropTypes.func.isRequired
};

export default DesignerPanel;