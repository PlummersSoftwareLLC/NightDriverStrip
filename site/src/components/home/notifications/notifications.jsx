import { useState } from 'react';
import Icon from '../../Icon';

const NotificationPanel = ({ notifications, clearNotifications }) => {
    const [open, setOpen] = useState(false);
    const total = notifications.reduce((s, n) => s + n.notifications.length, 0);

    return (
        <div className="notif-wrap">
            <button className="icon-btn" onClick={() => setOpen(o => !o)} title={`${total} notifications`}>
                <Icon name="bell" />
                <span className="notif-badge" />
            </button>
            {open && (
                <>
                    <div style={{position:'fixed',inset:0,zIndex:1499}} onClick={() => setOpen(false)} />
                    <div className="notif-popup">
                        <div className="notif-hdr">
                            <span style={{fontSize:'13px',fontWeight:600}}>{total} Notification{total !== 1 ? 's' : ''}</span>
                            <button className="icon-btn" style={{width:28,height:28}} onClick={() => { clearNotifications(); setOpen(false); }} title="Clear all">
                                <Icon name="close" size={16} />
                            </button>
                        </div>
                        {notifications.map((group, gi) => (
                            <div key={gi} className="notif-item">
                                <div style={{display:'flex',gap:6,marginBottom:2}}>
                                    <span className="notif-level">{group.level}</span>
                                    <span>{group.target}</span>
                                    <span style={{color:'var(--text-dim)'}}>{group.type}</span>
                                </div>
                                {Object.entries(
                                    group.notifications.reduce((acc, n) => {
                                        acc[n.notification] = (acc[n.notification] || 0) + 1;
                                        return acc;
                                    }, {})
                                ).map(([msg, count]) => (
                                    <div key={msg} style={{fontSize:'11px',color:'var(--text-dim)',marginTop:1}}>
                                        {count > 1 ? `${count}× ` : ''}{String(msg)}
                                    </div>
                                ))}
                            </div>
                        ))}
                    </div>
                </>
            )}
        </div>
    );
};

export default NotificationPanel;
