const msToTimeDisp = (millis, showMs=false) => {
    if (millis < 1000 && showMs) {
        return `${millis}ms`
    }  
    const seconds = Math.floor((millis / 1000) % 60);
    const minutes = Math.floor((millis / 1000 / 60));
    const hours = Math.floor((millis / 1000 / 60 / 60) % 24);        
    const parts = [];
    if (hours > 0) {
        parts.push(`${hours}${hours > 1 ? 'hrs' : 'hr'}`);
    }
    if (minutes > 0) {
        parts.push(`${minutes % 60}${minutes > 1 ? 'mins' : 'min'}`)  
    }
    if (seconds > 0 || (hours === 0 && minutes === 0))
        parts.push(seconds.toString().padStart(2, "0") + 's')
    return parts.join(' ')
}

export {msToTimeDisp}