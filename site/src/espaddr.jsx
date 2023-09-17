const httpPrefix = process.env.NODE_ENV === "development" ? "http://255.255.255.0" : undefined;
export default httpPrefix;