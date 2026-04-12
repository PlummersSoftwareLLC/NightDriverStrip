import { useState, useEffect, useContext } from "react";
import Icon from "../../../Icon";
import { EffectsContext } from "../../../../context/effectsContext";
import { msToTimeDisp } from "../../../../util/time";

const Countdown = ({ label }) => {
    const { remainingInterval, pinnedEffect, sync } = useContext(EffectsContext);
    const [timeRemaining, setTimeRemaining] = useState(0);

    useEffect(() => {
        if (pinnedEffect || !remainingInterval) return;
        const ref = Date.now() + remainingInterval;
        setTimeRemaining(remainingInterval);
        let synced = false;
        const id = setInterval(() => {
            const left = ref - Date.now();
            if (left >= 0) setTimeRemaining(left);
            if (left <= 100 && !synced) { synced = true; sync(); }
        }, 500);
        return () => clearInterval(id);
    }, [pinnedEffect, remainingInterval]);

    return (
        <div className="countdown">
            <span>{label}:</span>
            <span>
                {pinnedEffect
                    ? <Icon name="infinity" size={18} />
                    : msToTimeDisp(timeRemaining)
                }
            </span>
        </div>
    );
};

export default Countdown;
