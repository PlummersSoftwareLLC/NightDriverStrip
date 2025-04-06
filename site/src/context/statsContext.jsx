import { createContext, useEffect, useState } from "react";
import httpPrefix from "../espaddr";
import PropTypes from 'prop-types';

const StatsContext = createContext(undefined);
const statsEndpoint = `${httpPrefix !== undefined ? httpPrefix : ""}/statistics`;

const StatsProvider = ({ children }) => {
    const [matrixWidth, setMatrixWidth] = useState(64);
    const [matrixHeight, setMatrixHeight] = useState(32);
    const [framesSocket, setFramesSocket] = useState(false);
    const [effectsSocket, setEffectsSocket] = useState(false);
    useEffect(() => {
        const getDataFromDevice = async (params) => {
            try {
                const statistics = await fetch(statsEndpoint, params).then(r => r.json());
                setMatrixWidth(statistics.MATRIX_WIDTH);
                setMatrixHeight(statistics.MATRIX_HEIGHT);
                setFramesSocket(statistics.FRAMES_SOCKET);
                setEffectsSocket(statistics.EFFECTS_SOCKET);
            } catch(err) {
                console.debug("Aborted Statistics update");
            }
        };
        getDataFromDevice();
    });
    return (
        <StatsContext.Provider value={{matrixWidth, matrixHeight, effectsSocket, framesSocket}}>
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

export {StatsContext, StatsProvider};