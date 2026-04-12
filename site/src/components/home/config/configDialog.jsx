import { useState, useEffect } from "react";
import { RgbColorPicker } from "react-colorful";
import Icon from "../../Icon";
import httpPrefix from "../../../espaddr";

// Setting type enum — matches types.h/SettingSpec.SettingType
const T = { Integer: 0, PositiveBigInteger: 1, Float: 2, Boolean: 3, String: 4, Palette: 5, Color: 6, Slider: 7 };

const intToRGB = int => {
    const b = int.toString(2).padStart(24, '0');
    return { r: parseInt(b.slice(0, 8), 2), g: parseInt(b.slice(8, 16), 2), b: parseInt(b.slice(16, 24), 2) };
};
const RGBToInt = ({ r, g, b }) =>
    parseInt(r.toString(2).padStart(8,'0') + g.toString(2).padStart(8,'0') + b.toString(2).padStart(8,'0'), 2);

// ── Mini color picker dialog ──────────────────────────────────────────────────
const ColorPickerDialog = ({ title, initialColor, settingValue, setValue, onClose }) => {
    const [color, setColor] = useState(initialColor);
    return (
        <div className="modal-overlay" onClick={onClose}>
            <div className="modal" onClick={e => e.stopPropagation()} style={{maxWidth:300}}>
                <div className="modal-title">{title}</div>
                <div className="modal-body">
                    <RgbColorPicker color={color} onChange={setColor} />
                    <div className="rgb-row" style={{marginTop:10}}>
                        {['r','g','b'].map(ch => (
                            <input key={ch} className="cfg-input" type="number" min={0} max={255}
                                value={color[ch]}
                                onChange={e => {
                                    const v = Math.min(255, Math.max(0, parseInt(e.target.value) || 0));
                                    setColor(c => ({ ...c, [ch]: v }));
                                }}
                                style={{flex:1,width:0}}
                            />
                        ))}
                    </div>
                </div>
                <div className="modal-footer">
                    <button className="btn" onClick={() => setColor(intToRGB(settingValue))}>Reset</button>
                    <button className="btn" onClick={onClose}>Cancel</button>
                    <button className="btn primary" onClick={() => { setValue(RGBToInt(color)); onClose(); }}>OK</button>
                </div>
            </div>
        </div>
    );
};

// ── Per-setting input ─────────────────────────────────────────────────────────
const ConfigInput = ({ setting, updateData, updateError }) => {
    const [value,       setValue]       = useState(setting.value);
    const [error,       setError]       = useState(false);
    const [helperText,  setHelperText]  = useState(null);
    const [pickerIdx,   setPickerIdx]   = useState(-1);

    const desc = <span dangerouslySetInnerHTML={{ __html: setting.description }} />;

    useEffect(() => { updateError(setting.name, error); }, [error]);
    useEffect(() => {
        if (value !== setting.value) {
            if (value === '') {
                updateData(setting.name, setting.emptyAllowed ? value : setting.value);
            } else {
                updateData(setting.name, value);
            }
        } else {
            updateData(setting.name, undefined);
        }
    }, [value]);

    const numChange = (raw, min, max, floor = false) => {
        if (!raw && raw !== 0) { setValue(''); setError(true); setHelperText(`Must be ${min}–${max}`); return; }
        const n = floor ? Math.floor(Number(raw)) : Number(raw);
        if (isNaN(n) || n < min || n > max) {
            setError(true); setHelperText(`Must be between ${min.toLocaleString()} and ${max.toLocaleString()}`);
        } else {
            setError(false); setHelperText(null);
        }
        setValue(isNaN(n) ? '' : (floor ? Math.floor(n) : n));
    };

    const label = <label className="cfg-label">{setting.friendlyName}</label>;
    const helper = (
        <div className={`cfg-helper${error ? ' err' : ''}`}>
            {error && helperText ? helperText : desc}
        </div>
    );

    const readOnly = !!setting.readOnly;

    switch (setting.type) {
    case T.Integer: {
        const min = setting.minimumValue ?? -(2 ** 31);
        const max = setting.maximumValue ?? (2 ** 31 - 1);
        return <div className="cfg-field">{label}
            <input className={`cfg-input${error?' err':''}`} type="number" min={min} max={max}
                value={value} readOnly={readOnly}
                onChange={e => numChange(e.target.value, min, max, true)} />
            {helper}
        </div>;
    }
    case T.PositiveBigInteger: {
        const min = setting.minimumValue ?? 0;
        const max = setting.maximumValue ?? 2 ** 32;
        return <div className="cfg-field">{label}
            <input className={`cfg-input${error?' err':''}`} type="number" min={min} max={max}
                value={value} readOnly={readOnly}
                onChange={e => numChange(e.target.value, min, max, true)} />
            {helper}
        </div>;
    }
    case T.Float: {
        const min = setting.minimumValue ?? -3.4e38;
        const max = setting.maximumValue ??  3.4e38;
        return <div className="cfg-field">{label}
            <input className={`cfg-input${error?' err':''}`} type="number" min={min} max={max}
                value={value} readOnly={readOnly}
                onChange={e => numChange(e.target.value, min, max)} />
            {helper}
        </div>;
    }
    case T.Boolean:
        return <div className="cfg-field">
            <div className="cfg-checkbox-row">
                <input type="checkbox" disabled={readOnly} defaultChecked={!!setting.value}
                    onChange={e => setValue(e.target.checked)}
                    style={{accentColor:'var(--accent)',width:16,height:16}} />
                <label className="cfg-label" style={{textTransform:'none',letterSpacing:0}}>{setting.friendlyName}</label>
            </div>
            <div className="cfg-helper">{desc}</div>
        </div>;
    case T.String:
        return <div className="cfg-field">{label}
            <input className={`cfg-input${error?' err':''}`} type="text" value={value} readOnly={readOnly}
                onChange={e => {
                    const v = e.target.value;
                    if (v === '' && setting.emptyAllowed === false) {
                        setError(true); setHelperText('Empty value not allowed.');
                    } else { setError(false); setHelperText(null); }
                    setValue(v);
                }} />
            {helper}
        </div>;
    case T.Palette:
        return <div className="cfg-field">{label}
            <div className="palette-row">
                {value.map((colorInt, idx) => {
                    const rgb = intToRGB(colorInt);
                    return (
                        <div key={idx}>
                            <div className="color-swatch"
                                style={{backgroundColor:`rgb(${rgb.r},${rgb.g},${rgb.b})`}}
                                onClick={() => setPickerIdx(idx)} />
                            {pickerIdx === idx && (
                                <ColorPickerDialog
                                    title={setting.friendlyName}
                                    initialColor={rgb}
                                    settingValue={setting.value[idx]}
                                    setValue={v => setValue(arr => arr.map((old, i) => i === idx ? v : old))}
                                    onClose={() => setPickerIdx(-1)}
                                />
                            )}
                        </div>
                    );
                })}
            </div>
            <div className="cfg-helper">{desc}</div>
        </div>;
    case T.Color: {
        const rgb = intToRGB(value);
        return <div className="cfg-field">{label}
            <div className="color-preview-row">
                <div className="color-preview-bar"
                    style={{backgroundColor:`rgb(${rgb.r},${rgb.g},${rgb.b})`}}
                    onClick={() => setPickerIdx(0)} />
                <button className="icon-btn" onClick={() => setPickerIdx(0)} title="Pick color">
                    <Icon name="color_lens" />
                </button>
            </div>
            {pickerIdx === 0 && (
                <ColorPickerDialog
                    title={setting.friendlyName}
                    initialColor={rgb}
                    settingValue={setting.value}
                    setValue={v => { setValue(v); setPickerIdx(-1); }}
                    onClose={() => setPickerIdx(-1)}
                />
            )}
            <div className="cfg-helper">{desc}</div>
        </div>;
    }
    case T.Slider:
        return <div className="cfg-field">{label}
            <input className="cfg-slider" type="range"
                min={setting.minimumValue} max={setting.maximumValue}
                value={value} onChange={e => setValue(Number(e.target.value))} />
            <span style={{fontSize:12,color:'var(--text-dim)'}}>{value}</span>
            <div className="cfg-helper">{desc}</div>
        </div>;
    default:
        return <div className="cfg-field">{label}
            <input className="cfg-input" type="text" value={value} readOnly={readOnly}
                onChange={e => setValue(e.target.value)} />
            {helper}
        </div>;
    }
};

// ── Main dialog ───────────────────────────────────────────────────────────────
const ConfigDialog = ({ heading, effectIndex, open, setOpen, saveCallback }) => {
    const settingsUrl = `${httpPrefix !== undefined ? httpPrefix : ""}/settings${effectIndex !== undefined ? '/effect' : ''}`;
    const [content,       setContent]       = useState(null);
    const [saveData,      setSaveData]      = useState({});
    const [errorFields,   setErrorFields]   = useState(new Set());
    const [disableSubmit, setDisableSubmit] = useState(false);

    useEffect(() => {
        const spec    = `${settingsUrl}/specs${effectIndex !== undefined ? `?effectIndex=${effectIndex}` : ''}`;
        const current = `${settingsUrl}${effectIndex !== undefined ? `?effectIndex=${effectIndex}` : ''}`;

        const updateData = (name, value) => setSaveData(d => {
            const copy = { ...d };
            if (value !== undefined) copy[name] = value; else delete copy[name];
            return copy;
        });

        const updateError = (name, hasErr) => {
            setErrorFields(s => {
                const next = new Set(s);
                hasErr ? next.add(name) : next.delete(name);
                setDisableSubmit(next.size > 0);
                return next;
            });
        };

        Promise.all([fetch(spec).then(r => r.json()), fetch(current).then(r => r.json())])
            .then(([specs, cur]) => {
                const inputs = specs.map(s => {
                    if (s.name in cur) s.value = cur[s.name];
                    return <ConfigInput key={s.name} setting={s} updateData={updateData} updateError={updateError} />;
                });
                setContent(inputs);
            })
            .catch(() => setContent(<span style={{color:'var(--error)'}}>Failed to load settings.</span>));
    }, [effectIndex]);

    if (!open) return null;

    const submit = async () => {
        await fetch(settingsUrl, { method: 'POST', body: new URLSearchParams({ effectIndex, ...saveData }) }).then(r => r.json()).catch(() => {});
        saveCallback?.();
        setOpen(false);
    };

    const submitAndReboot = async () => {
        await submit();
        fetch(`${httpPrefix !== undefined ? httpPrefix : ""}/reset`, { method: 'POST', body: new URLSearchParams({ board: 1 }) });
    };

    return (
        <div className="modal-overlay" onClick={() => setOpen(false)}>
            <div className="modal" onClick={e => e.stopPropagation()}>
                <div className="modal-title">{heading} Configuration</div>
                <div className="modal-body">
                    {content ?? <span style={{color:'var(--text-dim)'}}>Loading…</span>}
                </div>
                <div className="modal-footer">
                    <button className="btn" onClick={() => setOpen(false)}>Cancel</button>
                    <button className="btn" disabled={disableSubmit} onClick={submit}>Apply</button>
                    <button className="btn primary" disabled={disableSubmit} onClick={submitAndReboot}>Apply &amp; Reboot</button>
                </div>
            </div>
        </div>
    );
};

export default ConfigDialog;
