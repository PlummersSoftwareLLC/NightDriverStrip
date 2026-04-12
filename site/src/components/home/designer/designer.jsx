import { useState, useContext, useEffect } from 'react';
import Icon from '../../Icon';
import Countdown from './countdown/countdown';
import Effect from './effect/effect';
import { EffectsContext } from '../../../context/effectsContext';
import { msToTimeDisp } from '../../../util/time';
import httpPrefix from '../../../espaddr';

const moveUrl = `${httpPrefix !== undefined ? httpPrefix : ""}/moveEffect`;

const DesignerPanel = ({ open, addNotification }) => {
    const { pinnedEffect, activeInterval, sync, effects } = useContext(EffectsContext);
    const [editing,      setEditing]      = useState(false);
    const [pendingInt,   setPendingInt]   = useState(Math.floor(activeInterval / 1000));
    const [requestRunning, setRunning]    = useState(false);
    const [gridLayout,   setGridLayout]   = useState(() => {
        const c = JSON.parse(localStorage.getItem('designerConfig') || '{}');
        return c.gridLayout !== undefined ? c.gridLayout : true;
    });
    const [showDisabled, setShowDisabled] = useState(() => {
        const c = JSON.parse(localStorage.getItem('designerConfig') || '{}');
        return c.showDisabled !== undefined ? c.showDisabled : true;
    });
    const [dragging,  setDragging]  = useState(undefined);
    const [dropTarget, setDropTarget] = useState(undefined);

    useEffect(() => { setPendingInt(Math.floor(activeInterval / 1000)); }, [activeInterval]);
    useEffect(() => {
        localStorage.setItem('designerConfig', JSON.stringify({ gridLayout, showDisabled }));
    }, [gridLayout, showDisabled]);

    const req = (url, opts, op) =>
        fetch(url, opts).catch(err => { addNotification('Error', op, url, err); throw err; });

    const navigateTo = idx => {
        setRunning(true);
        return req(`${httpPrefix !== undefined ? httpPrefix : ""}/currentEffect`,
            { method: 'POST', body: new URLSearchParams({ currentEffectIndex: idx }) }, 'navigateTo')
            .then(sync).finally(() => setRunning(false));
    };

    const effectEnable = (idx, enable) => {
        setRunning(true);
        return req(`${httpPrefix !== undefined ? httpPrefix : ""}/${enable ? 'enable' : 'disable'}Effect`,
            { method: 'POST', body: new URLSearchParams({ effectIndex: idx }) }, 'effectEnable')
            .then(sync).finally(() => setRunning(false));
    };

    const navigate = up => {
        setRunning(true);
        return req(`${httpPrefix !== undefined ? httpPrefix : ""}/${up ? 'nextEffect' : 'previousEffect'}`,
            { method: 'POST' }, 'navigate')
            .then(sync).finally(() => setRunning(false));
    };

    const saveInterval = secs => {
        setEditing(false);
        setRunning(true);
        return req(`${httpPrefix !== undefined ? httpPrefix : ""}/settings`,
            { method: 'POST', body: new URLSearchParams({ effectInterval: secs * 1000 }) }, 'setInterval')
            .then(sync).finally(() => setRunning(false));
    };

    if (!open) return null;
    if (!effects) return <div className="loading">Loading…</div>;

    const visible = effects.filter((e, i) => e.enabled || showDisabled ? true : false)
                           .map((e, i) => ({ ...e, origIdx: i }));

    return (
        <div className="designer-panel">
            <div className="effects-header">
                {/* Interval display / edit */}
                <div className="hdr-val">
                    <span>Interval:</span>
                    {editing ? (
                        <div style={{display:'flex',alignItems:'center',gap:4}}>
                            <input
                                className="hdr-val input"
                                type="number"
                                value={pendingInt}
                                min={1}
                                onChange={e => setPendingInt(e.target.value.replace(/\D/g, ''))}
                                onBlur={() => saveInterval(pendingInt)}
                                onKeyDown={e => e.key === 'Enter' && saveInterval(pendingInt)}
                                autoFocus
                                style={{width:70,background:'transparent',border:'none',borderBottom:'1px solid var(--accent)',color:'var(--text)',fontSize:13,padding:'2px 0',outline:'none'}}
                            />
                            <span style={{fontSize:12,color:'var(--text-dim)'}}>sec</span>
                        </div>
                    ) : (
                        <a href="#" onClick={e => { e.preventDefault(); setEditing(true); }}>
                            {pinnedEffect ? <Icon name="infinity" size={18} /> : msToTimeDisp(activeInterval)}
                        </a>
                    )}
                </div>
                <Countdown label="Remaining" />
                {effects.length > 1 && (
                    <div style={{display:'flex'}}>
                        <button className="icon-btn" disabled={requestRunning} onClick={() => navigate(false)} title="Previous">
                            <Icon name="skip_prev" />
                        </button>
                        <button className="icon-btn" disabled={requestRunning} onClick={() => navigate(true)} title="Next">
                            <Icon name="skip_next" />
                        </button>
                        <button className="icon-btn" disabled={requestRunning} onClick={() => sync()} title="Refresh">
                            <Icon name="refresh" />
                        </button>
                    </div>
                )}
                <div className="flex-grow" />
                <label className="label-sm">
                    <input type="checkbox" checked={showDisabled} onChange={() => setShowDisabled(v => !v)}
                        style={{accentColor:'var(--accent)'}} />
                    Show Disabled
                </label>
                <button className="icon-btn" onClick={() => setGridLayout(v => !v)} title="Toggle layout">
                    <Icon name={gridLayout ? 'list_view' : 'grid_view'} />
                </button>
            </div>

            <div
                className={gridLayout ? 'effects-grid' : 'effects-list'}
                onDragOver={e => { e.preventDefault(); setDropTarget(undefined); }}
                onDrop={e => {
                    e.preventDefault();
                    if (dragging !== undefined && dropTarget !== undefined && dragging !== dropTarget) {
                        fetch(moveUrl, { method: 'POST', body: new URLSearchParams({ effectIndex: dragging, newIndex: dropTarget }) })
                            .then(() => { setDragging(undefined); setDropTarget(undefined); sync(); });
                    }
                }}
            >
                {effects.map((effect, idx) =>
                    (effect.enabled || showDisabled) && (
                        <Effect
                            key={`effect-${idx}`}
                            effect={effect}
                            effectIndex={idx}
                            navigateTo={navigateTo}
                            requestRunning={requestRunning}
                            effectEnable={effectEnable}
                            gridLayout={gridLayout}
                            onDragStart={(e, i) => { setDragging(i); e.dataTransfer.setData('index', i); }}
                            onDragOver={(e, i) => { e.preventDefault(); if (i !== undefined) setDropTarget(i); }}
                        />
                    )
                )}
            </div>
        </div>
    );
};

export default DesignerPanel;
