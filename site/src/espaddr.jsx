const address = process.env.NODE_ENV === "development" ? "255.255.255.0" : undefined;
const httpPrefix = address ? `http://${address}` : undefined
const wsPrefix = address ? `ws://${address}/ws` : `ws://${window.location.host}/ws`

export default httpPrefix;
export { address, httpPrefix, wsPrefix }
