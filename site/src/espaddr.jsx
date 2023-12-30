const address = process.env.NODE_ENV === "development" ? "10.1.5.1" : undefined;
const httpPrefix = address ? `http://${address}`: undefined
const wsPrefix = address ? `ws://${address}/ws`: undefined

export default httpPrefix;
export {address, httpPrefix, wsPrefix}