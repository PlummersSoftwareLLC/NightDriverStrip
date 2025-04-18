import { createContext, useEffect, useState } from "react";
import httpPrefix from "../espaddr";
import PropTypes from 'prop-types';

const StatsContext = createContext(undefined);
const statsEndpoint = `${httpPrefix !== undefined ? httpPrefix : ""}/statistics/static`;

const StatsProvider = ({ children }) => {
    const [chipCores, setChipCores] = useState(1);
    const [chipModel, setChipModel] = useState('ESP32');
    const [chipSpeed, setChipSpeed] = useState(240);
    const [codeFree, setCodeFree] = useState(0);
    const [codeSize, setCodeSize] = useState(0);
    const [dmaSize, setDmaSize] = useState(0);
    const [effectsSocket, setEffectsSocket] = useState(false);
    const [flashSize, setFlashSize] = useState(0);
    const [framesSocket, setFramesSocket] = useState(false);
    const [heapSize, setHeapSize] = useState(0);
    const [matrixHeight, setMatrixHeight] = useState(32);
    const [matrixWidth, setMatrixWidth] = useState(64);
    const [progSize, setProgSize] = useState(0);
    const [psRamSize, setPsRamSize] = useState(0);

    useEffect(() => {
        const getDataFromDevice = async (params) => {
            try {
                const statistics = await fetch(statsEndpoint, params).then(r => r.json());
                setChipCores(statistics.CHIP_CORES);
                setChipModel(statistics.CHIP_MODEL);
                setChipSpeed(statistics.CHIP_SPEED);
                setCodeFree(statistics.CODE_FREE);
                setCodeSize(statistics.CODE_SIZE);
                setDmaSize(statistics.DMA_SIZE);
                setEffectsSocket(statistics.EFFECTS_SOCKET);
                setFlashSize(statistics.FLASH_SIZE);
                setFramesSocket(statistics.FRAMES_SOCKET);
                setHeapSize(statistics.HEAP_SIZE);
                setMatrixHeight(statistics.MATRIX_HEIGHT);
                setMatrixWidth(statistics.MATRIX_WIDTH);
                setProgSize(statistics.PROG_SIZE);
                setPsRamSize(statistics.PSRAM_SIZE);

            } catch (err) {
                console.debug("Aborted Statistics update");
            }
        };
        getDataFromDevice();
    });
    return (
        <StatsContext.Provider value={{
            chipCores,
            chipModel,
            chipSpeed,
            codeFree,
            codeSize,
            dmaSize,
            effectsSocket,
            flashSize,
            framesSocket,
            heapSize,
            matrixHeight,
            matrixWidth,
            progSize,
            psRamSize
        }}>
            {children}
        </StatsContext.Provider>
    );
};

StatsProvider.propTypes = {
    children: PropTypes.oneOfType([
        PropTypes.arrayOf(PropTypes.node),
        PropTypes.node
    ]).isRequired
};

export { StatsContext, StatsProvider };
