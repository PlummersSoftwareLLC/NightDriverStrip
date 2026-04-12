const CPU_COLORS = ['var(--cc1)','var(--cc2)','var(--cc3)','var(--cc4)'];
const MEM_COLORS = ['var(--cm1)','var(--cm2)','var(--cm3)','var(--cm4)'];

const BarStat = ({ name, rawvalue, ignored, idleField, category, detail }) => {
    const fields = Object.keys(rawvalue)
        .filter(k => !['name', ...ignored].includes(k))
        .sort((a, b) => a === idleField ? 1 : b === idleField ? -1 : a.localeCompare(b));

    const total   = fields.reduce((s, k) => s + (Number(rawvalue[k]) || 0), 0) || 1;
    const used    = fields.filter(k => k !== idleField).reduce((s, k) => s + (Number(rawvalue[k]) || 0), 0);
    const pct     = ((used / total) * 100).toFixed(0);
    const colors  = category === 'Memory' ? MEM_COLORS : CPU_COLORS;
    const w = detail ? 100 : 50;
    const h = detail ? 200 : 60;

    // Compute cumulative heights
    let cum = 0;
    const bars = fields.map((k, i) => {
        const frac = (Number(rawvalue[k]) || 0) / total;
        const barH = frac * h;
        const y    = h - cum - barH;
        cum += barH;
        const fill = k === idleField ? 'var(--cidle)' : colors[i % colors.length];
        return { k, y, barH, fill };
    });

    return (
        <div style={{display:'flex',flexDirection:'column',alignItems:'center',gap:2}}>
            <svg width={w} height={h} style={{display:'block'}}>
                <rect x={0} y={0} width={w} height={h} fill="rgba(0,0,0,0.15)" rx={2} />
                {bars.map(b => (
                    b.barH > 0 && <rect key={b.k} x={0} y={b.y} width={w} height={b.barH} fill={b.fill} fillOpacity={0.9} />
                ))}
            </svg>
            <span style={{fontSize:11,color:'var(--text-dim)'}}>{pct}%</span>
        </div>
    );
};

export default BarStat;
