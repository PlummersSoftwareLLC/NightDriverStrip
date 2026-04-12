import { useState, useEffect } from 'preact/hooks';

/* ── Helpers ─────────────────────────────────────────────────────────────── */
const fmtBytes = b => {
    if (!b || b <= 0) return '0';
    if (b >= 1048576) return `${(b / 1048576).toFixed(1)}M`;
    return `${(b / 1024).toFixed(0)}K`;
};
const clamp  = (v, lo = 0, hi = 100) => Math.max(lo, Math.min(hi, v || 0));
const fmtTime = s => s >= 60 ? `${Math.round(s / 60)}m` : `${Math.round(s)}s`;

/* ── Sparkline — HTML labels + non-scaling SVG chart ────────────────────── */
// SVG uses viewBox "0 0 100 CH" + preserveAspectRatio="none"
// x coords 0-100 stretch to fill column width; y coords are CSS pixels
// vector-effect keeps stroke at 1.5px regardless of scale
const CH = 34; // chart height px

const Sparkline = ({ data, color, labelFmt, fixedMax = null, refreshRate = 3 }) => {
    if (!data || data.length < 2) return <div className="spark-outer" />;

    const max  = fixedMax != null ? fixedMax : Math.max(...data, 0.1);
    const pad  = 2; // top/bottom inner padding in px
    const mkY  = v => +(CH - pad - (clamp(v, 0, max) / max) * (CH - pad * 2)).toFixed(1);
    const pts  = data.map((v, i) => [`${+((i / (data.length - 1)) * 100).toFixed(1)}`, mkY(v)]);
    const line = pts.map(([x, y]) => `${x},${y}`).join(' ');
    const area = `${pts.map(([x, y]) => `${x},${y}`).join(' ')} 100,${CH} 0,${CH}`;
    const midY = (mkY(0) + mkY(max)) / 2;
    const totalSecs = (data.length - 1) * refreshRate;

    return (
        <div className="spark-outer">
            {/* Y-axis labels */}
            <div className="spark-y">
                <span>{labelFmt(max)}</span>
                <span>{labelFmt(max / 2)}</span>
                <span>{labelFmt(0)}</span>
            </div>
            {/* Chart + X labels */}
            <div className="spark-body">
                <svg width="100%" height={CH}
                    viewBox={`0 0 100 ${CH}`}
                    preserveAspectRatio="none"
                    style="display:block;overflow:visible">
                    {/* grid */}
                    <line x1="0" y1={mkY(max)} x2="100" y2={mkY(max)}
                        stroke="var(--border)" strokeWidth="0.5"
                        style="vector-effect:non-scaling-stroke" strokeDasharray="3,4" />
                    <line x1="0" y1={midY} x2="100" y2={midY}
                        stroke="var(--border)" strokeWidth="0.5"
                        style="vector-effect:non-scaling-stroke" strokeDasharray="3,4" />
                    <line x1="0" y1={mkY(0)} x2="100" y2={mkY(0)}
                        stroke="var(--border)" strokeWidth="0.5"
                        style="vector-effect:non-scaling-stroke" />
                    <line x1="0" y1="0" x2="0" y2={CH}
                        stroke="var(--border)" strokeWidth="0.5"
                        style="vector-effect:non-scaling-stroke" />
                    {/* data */}
                    <polygon points={area} fill={color} fillOpacity="0.13" />
                    <polyline points={line} fill="none" stroke={color} strokeWidth="1.5"
                        strokeLinejoin="round" strokeLinecap="round"
                        style="vector-effect:non-scaling-stroke" />
                </svg>
                {/* X-axis labels */}
                <div className="spark-x">
                    <span>{totalSecs > 0 ? `-${fmtTime(totalSecs)}` : ''}</span>
                    <span>now</span>
                </div>
            </div>
        </div>
    );
};

/* ── Mini horizontal usage bar ───────────────────────────────────────────── */
const Bar = ({ pct, color }) => (
    <div className="sm-bar">
        <div className="sm-bar-fill" style={{ width: `${clamp(pct)}%`, background: color }} />
    </div>
);

/* ── Append-only history hook ────────────────────────────────────────────── */
const useHistory = (value, maxLen) => {
    const [hist, setHist] = useState([value ?? 0]);
    useEffect(() => {
        if (value == null) return;
        setHist(h => {
            const next = [...h, value];
            return next.length > maxLen ? next.slice(-maxLen) : next;
        });
    }, [value, maxLen]);
    return hist;
};

/* ── SystemMonitor ───────────────────────────────────────────────────────── */
const SystemMonitor = ({ raw, heapSize, dmaSize, psRamSize, fsSize, fsUsed, codeSize, flashSize, maxSamples, refreshRate = 3 }) => {
    if (!raw) return null;

    const c0       = raw.CPU_USED_CORE0 ?? 0;
    const c1       = raw.CPU_USED_CORE1 ?? 0;
    const heapUsed = heapSize  > 0 ? heapSize  - (raw.HEAP_FREE  ?? heapSize)  : 0;
    const dmaUsed  = dmaSize   > 0 ? dmaSize   - (raw.DMA_FREE   ?? dmaSize)   : 0;
    const psrUsed  = psRamSize > 0 ? psRamSize - (raw.PSRAM_FREE ?? psRamSize) : 0;
    const heapPct  = heapSize  > 0 ? (heapUsed / heapSize)  * 100 : 0;
    const dmaPct   = dmaSize   > 0 ? (dmaUsed  / dmaSize)   * 100 : 0;
    const psrPct   = psRamSize > 0 ? (psrUsed  / psRamSize) * 100 : 0;
    const fsPct    = fsSize    > 0 ? (fsUsed   / fsSize)    * 100 : 0;
    const flashPct = flashSize > 0 ? (codeSize / flashSize) * 100 : 0;

    const histC0   = useHistory(c0,               maxSamples);
    const histHeap = useHistory(heapUsed,          maxSamples);
    const histLed  = useHistory(raw.LED_FPS ?? 0, maxSamples);

    const cpuColor  = pct => pct > 80 ? '#f85149' : pct > 60 ? '#d29922' : 'var(--cstroke)';
    const memColor  = pct => pct > 85 ? '#f85149' : pct > 70 ? '#d29922' : 'var(--cmstroke)';
    const storColor = pct => pct > 90 ? '#f85149' : pct > 75 ? '#d29922' : 'var(--text-dim)';

    const fpsSrc = [
        { label: 'LED',    value: raw.LED_FPS    ?? 0, color: 'var(--green)'    },
        { label: 'SERIAL', value: raw.SERIAL_FPS ?? 0, color: 'var(--text-dim)' },
        { label: 'AUDIO',  value: raw.AUDIO_FPS  ?? 0, color: 'var(--accent)'   },
    ];

    return (
        <div className="sysmon">
            {/* ── CPU ── */}
            <div className="sysmon-col">
                <div className="sysmon-title">CPU</div>
                {[{ lbl: 'CORE0', pct: c0 }, { lbl: 'CORE1', pct: c1 }].map(({ lbl, pct }) => (
                    <div key={lbl} className="sysmon-row">
                        <span className="sm-lbl">{lbl}</span>
                        <Bar pct={pct} color={cpuColor(pct)} />
                        <span className="sm-val">{pct.toFixed(0)}%</span>
                    </div>
                ))}
                <div className="sm-sparkline">
                    <Sparkline data={histC0} color="var(--cstroke)"
                        fixedMax={100} labelFmt={v => `${Math.round(v)}%`}
                        refreshRate={refreshRate} />
                </div>
            </div>

            {/* ── Memory ── */}
            <div className="sysmon-col">
                <div className="sysmon-title">Memory</div>
                {[
                    { lbl: 'HEAP',  pct: heapPct, used: heapUsed, total: heapSize },
                    { lbl: 'DMA',   pct: dmaPct,  used: dmaUsed,  total: dmaSize  },
                    ...(psRamSize > 0 ? [{ lbl: 'PSRAM', pct: psrPct, used: psrUsed, total: psRamSize }] : []),
                ].map(({ lbl, pct, used, total }) => (
                    <div key={lbl} className="sysmon-row">
                        <span className="sm-lbl">{lbl}</span>
                        <Bar pct={pct} color={memColor(pct)} />
                        <span className="sm-val" title={`${fmtBytes(used)} / ${fmtBytes(total)}`}>
                            {fmtBytes(used)}
                        </span>
                    </div>
                ))}
                <div className="sm-sparkline">
                    <Sparkline data={histHeap} color="var(--cmstroke)"
                        labelFmt={fmtBytes} refreshRate={refreshRate} />
                </div>
            </div>

            {/* ── Storage ── */}
            <div className="sysmon-col">
                <div className="sysmon-title">Storage</div>
                {flashSize > 0 && (
                    <div className="sysmon-row">
                        <span className="sm-lbl">FW</span>
                        <Bar pct={flashPct} color={storColor(flashPct)} />
                        <span className="sm-val" title={`${fmtBytes(codeSize)} / ${fmtBytes(flashSize)}`}>
                            {fmtBytes(codeSize)}
                        </span>
                    </div>
                )}
                {fsSize > 0 && (
                    <div className="sysmon-row">
                        <span className="sm-lbl">FS</span>
                        <Bar pct={fsPct} color={storColor(fsPct)} />
                        <span className="sm-val" title={`${fmtBytes(fsUsed)} / ${fmtBytes(fsSize)}`}>
                            {fmtBytes(fsUsed)}
                        </span>
                    </div>
                )}
                {flashSize === 0 && fsSize === 0 && (
                    <div style={{fontSize:10,color:'var(--text-dim)',padding:'4px 0'}}>—</div>
                )}
            </div>

            {/* ── NightDriver FPS ── */}
            <div className="sysmon-col">
                <div className="sysmon-title">NightDriver</div>
                <div className="fps-grid">
                    {fpsSrc.map(({ label, value, color }) => (
                        <div key={label} className="fps-row">
                            <span className="fps-lbl">{label}</span>
                            <div style={{ display:'flex', alignItems:'baseline', gap:2 }}>
                                <span className="fps-num" style={{ color }}>{value}</span>
                                <span className="fps-unit">fps</span>
                            </div>
                        </div>
                    ))}
                </div>
                <div className="sm-sparkline">
                    <Sparkline data={histLed} color="var(--green)"
                        labelFmt={v => `${Math.round(v)}`}
                        refreshRate={refreshRate} />
                </div>
            </div>
        </div>
    );
};

export default SystemMonitor;
