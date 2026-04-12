import { useState } from "react";

// CSS variable → hex color mapping for chart fills
const CPU_COLORS  = ['var(--cc1)','var(--cc2)','var(--cc3)','var(--cc4)'];
const MEM_COLORS  = ['var(--cm1)','var(--cm2)','var(--cm3)','var(--cm4)'];

const AreaStat = ({ name, rawvalue, ignored, maxSamples, idleField, headerFields, category, detail, statsAnimateChange }) => {
    const activeKeys = Object.keys(rawvalue).filter(k => !ignored.includes(k) && rawvalue[k] !== undefined);
    const colors = category === 'Memory' ? MEM_COLORS : CPU_COLORS;

    const [history, setHistory] = useState([{ ...rawvalue, _ts: Date.now() }]);

    // Append new data point when rawvalue changes (via useMemo-like pattern)
    const lastVal = history[history.length - 1];
    const changed = activeKeys.some(k => lastVal[k] !== rawvalue[k]);
    if (changed) {
        // Schedule state update outside render
        Promise.resolve().then(() =>
            setHistory(h => {
                const next = [...h, { ...rawvalue, _ts: Date.now() }];
                return next.length > maxSamples ? next.slice(next.length - maxSamples) : next;
            })
        );
    }

    const w = detail ? 480 : 200;
    const h = detail ? 140 : 56;
    const pad = 2;
    const iw = w - pad * 2;
    const ih = h - pad * 2;
    const n  = history.length;

    // Build stacked-normalised areas
    const areas = activeKeys.map((key, ki) => {
        // For each time step compute cumulative fractions
        const pts_top = history.map((d, i) => {
            const total = activeKeys.reduce((s, k) => s + (Number(d[k]) || 0), 0) || 1;
            const cum = activeKeys.slice(0, ki + 1).reduce((s, k) => s + (Number(d[k]) || 0), 0);
            const x = pad + (i / Math.max(n - 1, 1)) * iw;
            const y = pad + ih - (cum / total) * ih;
            return `${x},${y}`;
        });
        const pts_bot = history.map((d, i) => {
            const total = activeKeys.reduce((s, k) => s + (Number(d[k]) || 0), 0) || 1;
            const cum = activeKeys.slice(0, ki).reduce((s, k) => s + (Number(d[k]) || 0), 0);
            const x = pad + (i / Math.max(n - 1, 1)) * iw;
            const y = pad + ih - (cum / total) * ih;
            return `${x},${y}`;
        }).reverse();
        const fill = key === idleField ? 'var(--cidle)' : colors[ki % colors.length];
        return { key, fill, points: [...pts_top, ...pts_bot].join(' ') };
    });

    const fmt = v => v !== undefined && !Number.isInteger(v) ? (isNaN(v) ? v : Number(v).toFixed(1)) : v;

    return (
        <div>
            {detail && (
                <div style={{marginBottom:4}}>
                    <span style={{fontSize:12,fontWeight:600}}>{name}</span>
                    {headerFields && headerFields.map(f =>
                        <span key={f} style={{fontSize:11,color:'var(--text-dim)',marginLeft:6}}>
                            {f}: {fmt(rawvalue[f])}
                        </span>
                    )}
                    <div style={{display:'flex',flexWrap:'wrap',gap:8,marginTop:4}}>
                        {activeKeys.filter(k => !ignored.includes(k)).map(k => (
                            <span key={k} style={{fontSize:11}}>
                                <span style={{color:'var(--text)'}}>{k}</span>
                                <span style={{color:'var(--text-dim)',marginLeft:2}}>{fmt(rawvalue[k])}</span>
                            </span>
                        ))}
                    </div>
                </div>
            )}
            <svg width={w} height={h} style={{display:'block'}}>
                <rect x={0} y={0} width={w} height={h} fill="rgba(0,0,0,0.15)" rx={2} />
                {areas.map(a => (
                    <polygon key={a.key} points={a.points} fill={a.fill} fillOpacity={0.85} stroke="none" />
                ))}
            </svg>
        </div>
    );
};

export default AreaStat;
