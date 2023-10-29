import {useState, useContext, useEffect} from 'react';
import {IconButton, Icon, Typography, Box, Link, ClickAwayListener, TextField, Checkbox, FormControlLabel} from '@mui/material';
import Countdown from './countdown/countdown';
import Effect from './effect/effect';
import designStyle from './style';
import httpPrefix from '../../../espaddr';
import PropTypes from 'prop-types';
import { EffectsContext} from '../../../context/effectsContext';

const moveEffectEndpoint = `${httpPrefix !== undefined ? httpPrefix : ""}/moveEffect`;
const DesignerPanel = ({ open, addNotification }) => {
    const config = JSON.parse(localStorage.getItem('designerConfig'));
    const {pinnedEffect, activeInterval, sync, effects} = useContext(EffectsContext);
    const [ editing, setEditing ] = useState(false);
    const [ requestRunning, setRequestRunning ] = useState(false);
    const [ pendingInterval, setPendingInterval ] = useState(activeInterval);
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

    const chipRequest = (url,options,operation) =>
        new Promise((resolve,reject) =>
            fetch(url,options)
                .then(resolve)
                .catch(err => {addNotification("Error",operation,err);reject(err);}));

    const navigateTo = (idx)=>{
        return new Promise((resolve,reject)=>{
            setRequestRunning(true);
            chipRequest(`${httpPrefix !== undefined ? httpPrefix : ""}/currentEffect`,{method:"POST", body: new URLSearchParams({currentEffectIndex:idx})}, "navigateTo")
                .then(resolve)
                .then(sync)
                .catch(reject)
                .finally(()=>setRequestRunning(false));
        });
    };

    const effectEnable = (idx,enable)=>{
        return new Promise((resolve,reject)=>{
            setRequestRunning(true);
            chipRequest(`${httpPrefix !== undefined ? httpPrefix : ""}/${enable?"enable":"disable"}Effect`,{method:"POST", body:new URLSearchParams({effectIndex:idx})},"effectEnable")
                .then(resolve)
                .then(sync)
                .catch(reject)
                .finally(()=>setRequestRunning(false));
        });
    };

    const navigate = (up)=>{
        return new Promise((resolve,reject)=>{
            setRequestRunning(true);
            chipRequest(`${httpPrefix !== undefined ? httpPrefix : ""}/${up ? "nextEffect" : "previousEffect"}`,{method:"POST"},"nvigate")
                .then(resolve)
                .then(sync)
                .catch(reject)
                .finally(()=>setRequestRunning(false));
        });
    };

    const updateEventInterval = (interval)=>{
        setEditing(false);
        return new Promise((resolve,reject)=>{
            setRequestRunning(true);
            return chipRequest(`${httpPrefix !== undefined ? httpPrefix : ""}/settings`,
                {
                    method:"POST",
                    body: new URLSearchParams({effectInterval:interval})
                },"updateEventInterval").then(resolve)
                .then(sync)
                .catch(reject)
                .finally(()=>setRequestRunning(false));
        });
    };

    const displayHeader = ()=>{
        return <Box sx={designStyle.effectsHeaderValue}>
            <Typography variant="little" color="textPrimary">Interval</Typography>:
            <Link href="#" variant="little" color="textSecondary" sx={pinnedEffect ? {height: "25px"}: {}} onClick={() => setEditing(true)}>{pinnedEffect ?<Icon>all_inclusive</Icon> : activeInterval}</Link>
        </Box>;
    };

    const editingHeader = ()=>{
        return <ClickAwayListener onClickAway={()=>{updateEventInterval(pendingInterval); sync();}}>
            <Box sx={designStyle.effectsHeaderValue}>
                <TextField label="Interval ms"
                    variant="outlined"
                    type="number"
                    defaultValue={activeInterval}
                    onChange={event => setPendingInterval(event.target.value)} />
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
            onDrop={(event, index) => {handleDrop(event, index, setDragging, setDropTarget, dragging, dropTarget, sync)}}>            
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

function handleDrop(event, index, setDragging, setDropTarget, effectIndex, newIndex, sync) {
    event.preventDefault();
    if(effectIndex !== undefined && newIndex !== undefined && effectIndex !== newIndex) {
        return fetch(moveEffectEndpoint, {method:"POST", body:new URLSearchParams({effectIndex, newIndex})}).then(() => {
            setDragging(undefined);
            setDropTarget(undefined);
            sync();
        });
    }
}

DesignerPanel.propTypes = {
    open: PropTypes.bool.isRequired, 
    addNotification: PropTypes.func.isRequired
};

export default DesignerPanel;