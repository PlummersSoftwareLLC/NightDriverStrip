import { useState, useEffect, useContext, useRef } from "react";
import { wsPrefix } from "../../../../espaddr";
import { StatsContext } from "../../../../context/statsContext";
import { Dialog, DialogActions, DialogContent, DialogTitle, Button } from "@mui/material";
import { intToRGB } from "../../../../util/color"
import Canvas from "../canvas";

const PreviewDialog = ({ open, onClose }) => {
    const { matrixWidth, matrixHeight, framesSocket } = useContext(StatsContext);

    const ws = useRef(null);
    const [frame, setFrame] = useState([])
    const [reconnect, setReconnect] = useState(true); // flip boolean to trigger a reconnect, if undefined connection should be closed, don't reconnect. 
    // Setup Websocket reference once. 
    useEffect(() => {
        ws.current = new WebSocket(`${wsPrefix}/frames`);

        ws.current.onopen = () => {
            console.debug('frames websocket connected');
        };

        ws.current.onclose = (x) => {
            console.log('reconnecting', 'reconnect', reconnect, 'open', open, 'x', x);
            if (open) {
                setReconnect((r) => !r);
            }
        };

        const wsCurrent = ws.current
        return () => {
            console.log('closing ws');
            if (wsCurrent && wsCurrent.readyState == WebSocket.OPEN) {
                wsCurrent.close();
            }
        }
    }, [reconnect]);

    useEffect(() => {
        if (!ws.current) {
            return;
        }
        ws.current.onmessage = (event) => {
            if (!open) {
                return;
            }
            try {
                const { colorData } = JSON.parse(event.data);
                setFrame(colorData)
            } catch (err) {
                console.log('error proccessing ws message', err);
                console.debug('data:', event.data)
            }
        }
    }, [open, reconnect])
    const draw = (ctx, ) => {
        if (frame.length > 0 && matrixWidth && matrixHeight) {
            const imageData = ctx.createImageData(matrixWidth, matrixHeight);
            let pixel = 0;
            for (let i = 0; i < frame.length; i++) {
                const [count, colorInt] = frame[i].split("|");
                const color = colorInt ? intToRGB(+colorInt) : { r: 0, b: 0, g: 0 };
                for(let j = 0; j < count; j++) {
                    imageData.data[pixel] = color.r;
                    imageData.data[pixel + 1] = color.g;
                    imageData.data[pixel + 2] = color.b;
                    imageData.data[pixel + 3] = 255; // A in a RGBA notation, Could be replaced with a brightness value later.            
                    pixel += 4;
                }
                
            }
            const newCanvas = document.createElement("canvas");
            newCanvas.setAttribute('width', matrixWidth);
            newCanvas.setAttribute('height', matrixHeight);
            newCanvas.getContext("2d").putImageData(imageData, 0, 0);
            ctx.drawImage(newCanvas, 0, 0);
        }
    }

    return <Dialog open={open} onClose={onClose} fullWidth={true}>
        <DialogTitle>{"Preview"}</DialogTitle>
        <DialogContent sx={{ textAlign: 'center' }}>
            <Canvas draw={draw} scale={5} width={matrixWidth * 5} height={matrixHeight * 5}></Canvas>
        </DialogContent>
        <DialogActions>
            <Button onClick={onClose}>close</Button>
        </DialogActions>
    </Dialog>
}

export default PreviewDialog;