const StaticStatsPanel = ({ stat, name, detail }) => (
    <div className="stat-block">
        <span className="stat-bname">{name}</span>
        {detail
            ? Object.entries(stat.stat).map(([k, v]) => (
                <div className="stat-row" key={k}>
                    <span className="stat-key">{k}:</span>
                    <span className="stat-val">{v}</span>
                </div>
            ))
            : (stat.headerFields || []).map(k => (
                <span className="stat-val" key={k}>{stat.stat[k]}</span>
            ))
        }
    </div>
);

export default StaticStatsPanel;
