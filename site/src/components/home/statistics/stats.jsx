import { useState, useEffect, useContext } from 'react';
import Icon from '../../Icon';
import SystemMonitor from './SystemMonitor';
import { StatsContext } from '../../../context/statsContext';
import httpPrefix from '../../../espaddr';

const dynamicUrl = `${httpPrefix !== undefined ? httpPrefix : ""}/statistics/dynamic`;
const fmtBytes = b => (!b || b <= 0) ? '0' : b >= 1048576 ? `${(b/1048576).toFixed(1)} MB` : `${(b/1024).toFixed(0)} KB`;

const StatsPanel = ({ open, addNotification }) => {
    const cfg = JSON.parse(localStorage.getItem('statsConfig') || '{}');
    const [rawData,      setRawData]      = useState(null);
    const [refreshRate,  setRefreshRate]  = useState(cfg.statsRefreshRate ?? 3);
    const [maxSamples,   setMaxSamples]   = useState(cfg.maxSamples ?? 50);
    const [settingsOpen, setSettingsOpen] = useState(false);
    const [lastRefresh,  setLastRefresh]  = useState(0);

    const { heapSize, dmaSize, psRamSize, chipCores, chipModel, chipSpeed, codeSize, codeFree, flashSize, fsSize, fsUsed, buildInfo } = useContext(StatsContext);

    useEffect(() => {
        localStorage.setItem('statsConfig', JSON.stringify({ statsRefreshRate: refreshRate, maxSamples }));
    }, [refreshRate, maxSamples]);

    useEffect(() => {
        if (!open) return;
        const ctrl = new AbortController();
        fetch(dynamicUrl, { signal: ctrl.signal })
            .then(r => r.json())
            .then(s => setRawData(s))
            .catch(() => {});

        const id = setTimeout(() => setLastRefresh(Date.now()), refreshRate * 1000);
        return () => { clearTimeout(id); ctrl.abort(); };
    }, [refreshRate, lastRefresh, open]);

    if (!open) return null;

    const pkgItems = [
        { key: 'Model',  val: chipModel },
        { key: 'Cores',  val: chipCores },
        { key: 'Speed',  val: chipSpeed ? `${chipSpeed} MHz` : '—' },
        { key: 'Flash',  val: fmtBytes(flashSize) },
        { key: 'Code',   val: fmtBytes(codeSize) },
        { key: 'Free',   val: fmtBytes(codeFree) },
        ...(buildInfo ? [{ key: 'Last Updated', val: buildInfo }] : []),
    ];

    return (
        <div className="stats-panel">
            {/* Header row */}
            <div className="section-head">
                <span className="section-head-title">Statistics</span>
                <button className="icon-btn" style={{width:26,height:26}} onClick={() => setSettingsOpen(true)} title="Stats settings">
                    <Icon name="settings" size={15} />
                </button>
            </div>

            {/* Package info bar */}
            <div className="pkg-bar">
                {pkgItems.map(({ key, val }) => (
                    <div key={key} className="pkg-bar-item">
                        <span className="pkg-item-key">{key}</span>
                        <span className="pkg-item-val">{val ?? '—'}</span>
                    </div>
                ))}
            </div>

            {/* CPU · Memory · Storage · FPS — combined horizontal strip */}
            {rawData
                ? <SystemMonitor
                    raw={rawData}
                    heapSize={heapSize}
                    dmaSize={dmaSize}
                    psRamSize={psRamSize}
                    fsSize={fsSize}
                    fsUsed={fsUsed}
                    codeSize={codeSize}
                    flashSize={flashSize}
                    maxSamples={maxSamples}
                    refreshRate={refreshRate}
                  />
                : <div className="loading">Fetching…</div>
            }

            {/* Settings modal */}
            {settingsOpen && (
                <div className="modal-overlay" onClick={() => setSettingsOpen(false)}>
                    <div className="modal" onClick={e => e.stopPropagation()} style={{maxWidth:340}}>
                        <div className="modal-title">Stats Settings</div>
                        <div className="modal-body">
                            <div className="cfg-field">
                                <label className="cfg-label">Refresh rate (seconds)</label>
                                <input className="cfg-input" type="number" min={1} max={60} value={refreshRate}
                                    onChange={e => setRefreshRate(Math.min(60, Math.max(1, Number(e.target.value))))} />
                            </div>
                            <div className="cfg-field">
                                <label className="cfg-label">Sparkline history (samples)</label>
                                <input className="cfg-input" type="number" min={5} max={250} value={maxSamples}
                                    onChange={e => setMaxSamples(Math.min(250, Math.max(5, Number(e.target.value))))} />
                            </div>
                        </div>
                        <div className="modal-footer">
                            <button className="btn primary" onClick={() => setSettingsOpen(false)}>Done</button>
                        </div>
                    </div>
                </div>
            )}
        </div>
    );
};

export default StatsPanel;
