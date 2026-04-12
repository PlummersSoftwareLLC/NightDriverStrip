import { useState, useEffect, useContext, useRef } from "react";
import { wsPrefix } from "../../../../espaddr";
import { StatsContext } from "../../../../context/statsContext";
import Canvas from "../canvas";

const PreviewDialog = ({ open, onClose }) => {
    const { matrixWidth, matrixHeight } = useContext(StatsContext);
    const ws = useRef(null);
    const [frame, setFrame] = useState([]);
    const [reconnect, setReconnect] = useState(true);

    useEffect(() => {
        ws.current = new WebSocket(`${wsPrefix}/frames`);
        ws.current.binaryType = "arraybuffer";
        ws.current.onclose = () => { if (open) setReconnect(r => !r); };
        const sock = ws.current;
        return () => { if (sock.readyState === WebSocket.OPEN) sock.close(); };
    }, [reconnect]);

    useEffect(() => {
        if (!ws.current) return;
        ws.current.onmessage = event => {
            if (!open) return;
            try { setFrame(new Uint8Array(event.data)); } catch (_) {}
        };
    }, [open, reconnect]);

    const draw = ctx => {
        if (frame.length > 0 && matrixWidth && matrixHeight) {
            const img = ctx.createImageData(matrixWidth, matrixHeight);
            let px = 0;
            for (let i = 0; i < frame.length; i += 3) {
                img.data[px] = frame[i]; img.data[px+1] = frame[i+1];
                img.data[px+2] = frame[i+2]; img.data[px+3] = 255;
                px += 4;
            }
            const tmp = document.createElement("canvas");
            tmp.width = matrixWidth; tmp.height = matrixHeight;
            tmp.getContext("2d").putImageData(img, 0, 0);
            ctx.drawImage(tmp, 0, 0);
        }
    };

    if (!open) return null;

    const scale = 10;
    return (
        <div className="modal-overlay" onClick={onClose}>
            <div className="modal wide" onClick={e => e.stopPropagation()}>
                <div className="modal-title">Frame Preview</div>
                <div className="modal-body" style={{textAlign:'center'}}>
                    <Canvas draw={draw} scale={scale} width={matrixWidth * scale} height={matrixHeight * scale} />
                </div>
                <div className="modal-footer">
                    <button className="btn primary" onClick={onClose}>Close</button>
                </div>
            </div>
        </div>
    );
};

export default PreviewDialog;
