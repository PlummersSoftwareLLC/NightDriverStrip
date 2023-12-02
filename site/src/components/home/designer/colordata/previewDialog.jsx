import { useState, useEffect } from "react";
import { wsPrefix } from "../../../../espaddr";
import {Box,  Dialog, DialogActions, DialogContent, DialogTitle, Button} from "@mui/material";
import { intToRGB, RGBToInt } from "../../../../util/color"
 
// Mock test colors for testing. TODO Remove
const tempColor1 = RGBToInt({r: 255, g:0, b:0})
const tempColor2 = RGBToInt({r: 0, g:255, b:0})
const tempColor3 = RGBToInt({r: 0, g:0, b:255})
const tempColor4 = RGBToInt({r: 255, g:255, b:0})
const tempColor5 = RGBToInt({r: 255, g:0, b:255})
const tempColor6 = RGBToInt({r: 0, g:255, b:255})
const tempColor7 = RGBToInt({r: 255, g:255, b:255})
const tempColor8 = RGBToInt({r: 0, g:0, b:0})
// End mock test colors. TODO Remove

const PreviewDialog = ({open, onClose}) => {
    const [connected, setConnected] = useState(false);
    let testFrame = new Array(64*32)
    // Test frame only used for testing until ws is implemented. TODO Remove
    for(let i = 0; i < 4; i++) {
        const start = 512*i;
        testFrame = testFrame.fill(tempColor1, start+0,start+64);
        testFrame = testFrame.fill(tempColor2, start+64,start+128);
        testFrame = testFrame.fill(tempColor3, start+128,start+192);
        testFrame = testFrame.fill(tempColor4, start+192,start+256);
        testFrame = testFrame.fill(tempColor5, start+256,start+320);
        testFrame = testFrame.fill(tempColor6, start+320,start+384);
        testFrame = testFrame.fill(tempColor7, start+384,start+448);
        testFrame = testFrame.fill(tempColor8, start+448,start+512);
    }
    console.log('testFrame:', testFrame)
    //END Test Frame. TODO Remove

    const [frame, setFrame] = useState(testFrame)    
    useEffect(() => {
        if(!connected) {
            const ws = {}; //Temporary to disable ws logic
            // const ws = new WebSocket(`${wsPrefix}/colorData`); // TODO Uncomment this when ws is setup

            ws.onopen = () => {
                setConnected(true);
            };
            ws.onclose = () => {
                setConnected(false)
            }
            ws.onmessage = (event) => {
                try {
                    console.log('Websocket message');
                    const json = JSON.parse(event.data);
                    console.log('Todo process this data and push it into the frame ', json);
                    setFrame(json) // TODO any data packing to get an array of colors. 
                } catch(err) {
                    console.log('error proccessing ws message', err);
                    console.log('data:', event.data)
                }
            }
            // TODO Uncomment below when WS is setup
            // return () => ws.close();
        }
    }, [connected])

    return <Dialog open={open} onClose={onClose} fullWidth={true}>
        <DialogTitle>{"Preview"}</DialogTitle>
        <DialogContent>
            <Frame colors={frame} width={64} height={32}></Frame>
        </DialogContent>
        <DialogActions>
            <Button onClick={onClose}>close</Button>
        </DialogActions>
    </Dialog>
}

const Frame = ({colors, width, height}) => {
    const rows = [] 
    for(let y = 0; y < height; y++) {
        const row = [];
        const startingPixel = y*width;
        for(let x = 0; x < width; x++) {
            const color = intToRGB(colors[startingPixel + x])
            // TODO Improve pixel size calculation
            row.push(<Box sx={{display: 'table-cell', width: `10px`, height: `10px`, backgroundColor: `rgb(${color.r},${color.g},${color.b})`}}/>)
        }
        console.log('pushing row of ', row.length, 'elements')
        rows.push(<Box sx={{display: 'table-row', height: '1vh'}}>{row}</Box>)
    } 
    return <Box sx={{width: '100%', height: '100%'}}>{rows}</Box>
}

export default PreviewDialog;