import { createContext, useEffect, useState } from "react";
import httpPrefix from "../espaddr";

const StatsContext = createContext(undefined);
const statsEndpoint = `${httpPrefix !== undefined ? httpPrefix : ""}/statistics/static`;

const StatsProvider = ({ children }) => {
    const [stats, setStats] = useState({
        buildInfo: null,
        chipCores: 1, chipModel: 'ESP32', chipSpeed: 240,
        codeFree: 0, codeSize: 0, dmaSize: 0,
        effectsSocket: false, flashSize: 0, framesSocket: false,
        fsSize: 0, fsUsed: 0,
        heapSize: 0, matrixHeight: 32, matrixWidth: 64,
        progSize: 0, psRamSize: 0,
    });

    useEffect(() => {
        fetch(statsEndpoint)
            .then(r => r.json())
            .then(s => setStats({
                chipCores:    s.CHIP_CORES,
                chipModel:    s.CHIP_MODEL,
                chipSpeed:    s.CHIP_SPEED,
                codeFree:     s.CODE_FREE,
                codeSize:     s.CODE_SIZE,
                dmaSize:      s.DMA_SIZE,
                buildInfo:    s.BUILD_INFO ?? null,
                effectsSocket:s.EFFECTS_SOCKET,
                flashSize:    s.FLASH_SIZE,
                framesSocket: s.FRAMES_SOCKET,
                fsSize:       s.FS_SIZE ?? 0,
                fsUsed:       s.FS_USED ?? 0,
                heapSize:     s.HEAP_SIZE,
                matrixHeight: s.MATRIX_HEIGHT,
                matrixWidth:  s.MATRIX_WIDTH,
                progSize:     s.PROG_SIZE,
                psRamSize:    s.PSRAM_SIZE,
            }))
            .catch(() => {});
    }, []); // ← runs once on mount only

    return <StatsContext.Provider value={stats}>{children}</StatsContext.Provider>;
};

export { StatsContext, StatsProvider };
