import { useState, useContext } from 'react';
import Icon from '../Icon';
import { EffectsContext } from '../../context/effectsContext';
import { StatsContext } from '../../context/statsContext';
import StatsPanel from './statistics/stats';
import DesignerPanel from './designer/designer';
import ConfigDialog from './config/configDialog';
import PreviewDialog from './designer/colordata/previewDialog';
import NotificationPanel from './notifications/notifications';
import httpPrefix from '../../espaddr';

const resetUrl = `${httpPrefix !== undefined ? httpPrefix : ""}/reset`;

const MainApp = () => {
    const [theme,    setTheme]    = useState(localStorage.getItem('theme') || 'dark');
    const [drawer,   setDrawer]   = useState(false);
    const [showStats,   setStats]   = useState(() => {
        const c = JSON.parse(localStorage.getItem('config') || '{}');
        return c.stats !== undefined ? c.stats : true;
    });
    const [showDesign,  setDesign]  = useState(() => {
        const c = JSON.parse(localStorage.getItem('config') || '{}');
        return c.designer !== undefined ? c.designer : true;
    });
    const [settings, setSettings] = useState(false);
    const [preview,  setPreview]  = useState(false);
    const [devCtrl,  setDevCtrl]  = useState(null); // anchor element
    const [notifications, setNotifications] = useState([]);

    const { sync } = useContext(EffectsContext);
    const { framesSocket } = useContext(StatsContext);

    const toggleTheme = () => {
        const next = theme === 'dark' ? 'light' : 'dark';
        setTheme(next);
        localStorage.setItem('theme', next);
        document.documentElement.setAttribute('data-theme', next);
    };

    const saveLayout = (stats, designer) => {
        localStorage.setItem('config', JSON.stringify({ stats, designer }));
    };

    const addNotification = (level, type, target, notification) => {
        setNotifications(prev => {
            const group = prev.find(n => n.level === level && n.type === type && n.target === target)
                       || { level, type, target, notifications: [] };
            group.notifications.push({ date: new Date(), notification });
            return [...prev.filter(n => n !== group), group];
        });
    };

    const navItems = [
        { label: 'Home',       icon: 'home',        active: showDesign,  toggle: () => { const v = !showDesign; setDesign(v);  saveLayout(showStats, v); } },
        { label: 'Statistics', icon: 'area_chart',  active: showStats,   toggle: () => { const v = !showStats;  setStats(v);   saveLayout(v, showDesign); } },
    ];

    return (
        <>
            {/* App bar */}
            <header className={`appbar${drawer ? ' open' : ''}`}>
                <button className="icon-btn" onClick={() => setDrawer(d => !d)} aria-label="toggle drawer">
                    <Icon name={drawer ? 'chevron_left' : 'menu'} />
                </button>
                <span className="appbar-title">NightDriverStrip</span>
                {notifications.length > 0 &&
                    <NotificationPanel notifications={notifications} clearNotifications={() => setNotifications([])} />
                }
            </header>

            {/* Side drawer */}
            <nav className={`drawer${drawer ? '' : ' closed'}`}>
                <div className="drawer-header">
                    <div className="drawer-mode">
                        <button className="icon-btn" onClick={toggleTheme} title="Toggle theme">
                            <Icon name={theme === 'dark' ? 'dark_mode' : 'light_mode'} />
                        </button>
                        <span className="nav-label" style={{fontSize:'13px'}}>{theme === 'dark' ? 'Dark' : 'Light'} mode</span>
                    </div>
                    <button className="icon-btn" onClick={() => setDrawer(false)}>
                        <Icon name="chevron_left" />
                    </button>
                </div>
                <hr className="divider" />
                <ul className="drawer-nav">
                    {navItems.map(item => (
                        <li key={item.icon} className="drawer-nav-item">
                            <button
                                className={`icon-btn${item.active ? ' active' : ''}`}
                                onClick={item.toggle}
                                title={item.label}
                            >
                                <Icon name={item.icon} />
                            </button>
                            <span className="nav-label" style={{fontSize:'13px'}}>{item.label}</span>
                        </li>
                    ))}
                    <li className="drawer-nav-item">
                        <button className="icon-btn" onClick={() => setSettings(s => !s)} title="Settings">
                            <Icon name="settings" />
                        </button>
                        <span className="nav-label" style={{fontSize:'13px'}}>Settings</span>
                    </li>
                    <li className="drawer-nav-item">
                        <button
                            className="icon-btn"
                            disabled={!framesSocket}
                            onClick={() => setPreview(p => !p)}
                            title="Preview frames"
                        >
                            <Icon name="smart_display" />
                        </button>
                        <span className="nav-label" style={{fontSize:'13px'}}>Preview</span>
                    </li>
                    <li className="drawer-nav-item" onClick={e => setDevCtrl(devCtrl ? null : e.currentTarget)}>
                        <button className="icon-btn" title="Device control">
                            <Icon name="power" />
                        </button>
                        <span className="nav-label" style={{fontSize:'13px'}}>Device Control</span>
                    </li>
                </ul>
            </nav>

            {/* Main content */}
            <main className={`main-content${drawer ? ' open' : ''}`}>
                <StatsPanel  open={showStats}  addNotification={addNotification} />
                <DesignerPanel open={showDesign} addNotification={addNotification} />
            </main>

            {/* Device control menu */}
            {devCtrl && (
                <>
                    <div style={{position:'fixed',inset:0,zIndex:1399}} onClick={() => setDevCtrl(null)} />
                    <div className="dev-ctrl" style={{top: devCtrl.getBoundingClientRect().bottom + 4, left: devCtrl.getBoundingClientRect().left}}>
                        <button className="btn" onClick={() => {
                            fetch(resetUrl, {method:'POST', body: new URLSearchParams({board:1})});
                            setDevCtrl(null);
                        }}>Reboot Device</button>
                        <button className="btn" onClick={() => {
                            fetch(resetUrl, {method:'POST', body: new URLSearchParams({board:1, deviceConfig:1})});
                            setDevCtrl(null);
                        }}>Reset Device Config</button>
                        <button className="btn" onClick={() => {
                            fetch(resetUrl, {method:'POST', body: new URLSearchParams({board:1, effectsConfig:1})});
                            setDevCtrl(null);
                        }}>Reset Effect Settings</button>
                    </div>
                </>
            )}

            {settings && <ConfigDialog heading="Device Settings" open={settings} setOpen={setSettings} saveCallback={sync} />}
            {preview  && <PreviewDialog open={preview} onClose={() => setPreview(false)} />}
        </>
    );
};

export default MainApp;
