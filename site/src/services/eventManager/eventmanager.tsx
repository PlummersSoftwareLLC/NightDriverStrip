
interface IEventManager {
    subscribe: (eventName: any, handler: any) => string;
    unsubscribe: (eventId: any) => void;
    emit: (eventName: string, data?: any, entityId?: string) => void;
}

const subscribers:Map<string,Map<string,Function>> = new Map<string,Map<string,Function>>();

export const eventManager = ():IEventManager => {
    
    const generateId = ()=>{
        const mask="xxxxxxxx-xxxxxxxx-xxxxxxxx-xxxxxxxx";
        const lexicon = "0123456789abcdef";

        return Array.from(mask).map(char=>char==='x'?lexicon[Math.floor(Math.random()*lexicon.length)]:char).join("");
    };

    const emit = (eventName: string, data?:any, entityId?:string) => subscribers.has(eventName) && 
                                                Array.from(subscribers?.get(eventName)?.entries() || [])?.filter(entry => 
                                                    (entityId === undefined || entityId == entry[0]))
                                                     .forEach(entry => entry[1](data));
    return {
        subscribe: (eventName, handler) => {
            {
                const eventId = generateId();

                if (subscribers.has(eventName)) {
                    subscribers?.get(eventName)?.set(eventId, handler);
                } else {
                    subscribers.set(eventName, new Map().set(eventId,handler));
                }
            
                emit("subscription",{eventName,eventId});
                return eventId;
            }
        },
        unsubscribe: (eventId) => {
            Array.from(subscribers.entries())
                .forEach(entry=>Array.from(entry[1])
                    .filter(map=>map[0]===eventId)
                    .forEach(_map=>entry[1].delete(eventId)))
        },
        emit
    };
};