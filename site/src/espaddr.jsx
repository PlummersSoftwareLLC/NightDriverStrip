const httpPrefix = process.env.NODE_ENV === "development" ? "http://10.1.5.1" : undefined;
export default httpPrefix;