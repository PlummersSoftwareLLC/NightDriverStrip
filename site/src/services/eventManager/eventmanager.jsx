const subscribers = new Map([]);    

const eventManager = () => {
    
    const generateId = ()=>{
        const mask="xxxxxxxx-xxxxxxxx-xxxxxxxx-xxxxxxxx";
        const lexicon = "0123456789abcdef";

        return Array.from(mask).map(char=>char==='x'?lexicon[Math.floor(Math.random()*lexicon.length)]:char).join("");
    };

    const emit = (eventName, data, entityId) => subscribers.has(eventName) && 
                                                Array.from(subscribers.get(eventName).entries()).forEach(entry => 
                                                    (entityId === undefined || entityId == entry[0]) && entry[1](data));
    return {
        subscribe: (eventName, handler) => {
            {
                const eventId = generateId();

                if (subscribers.has(eventName)) {
                    const subs = subscribers.get(eventName);
                    !Array.from(subs.values()).some(val => val.toString() === handler.toString()) && subs.set(eventId, handler);
                } else {
                    subscribers.set(eventName, new Map().set(eventId,handler));
                }
            
                emit("subscription",{eventName,eventId});
                return eventId;
            }
        },
        unsubscribe: (eventId) => Array.from(subscribers.entries()).forEach(entry=>Array.from(entry[1]).forEach(map=>map[0]===eventId && entry[1].delete(eventId))),
        emit
    };
};