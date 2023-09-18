const areaChartStyle = {
    root: {
        "display": "flex",
        flexDirection: "column",
        flexWrap: "wrap",
        alignContent: "flex-start",
        justifyContent: "flex-start",
        alignItems: "stretch"
    },
    header: {
        "display": "flex",
        flexDirection: "column",
        flexWrap: "nowrap",
        justifyContent: "center",
        alignItems: "stretch"
    },
    headerLine: {
        "display": "flex",
        flexDirection: "row",
        flexWrap: "nowrap",
        justifyContent: "space-between",
        alignItems: "center",
        "width": "100%"
    },
    headerField: {
        "display": "flex",
        flexDirection: "row",
        flexWrap: "nowrap",
        justifyContent: "center",
        alignItems: "center",
        "width": "inherit"
    },
    stats: {
        "display": "flex",
        flexDirection: "row",
        flexWrap: "nowrap",
        justifyContent: "center",
        alignItems: "center",
        "padding": "0px"
    },
    stat: {
        "display": "flex",
        "padding": "0px",
        flexWrap: "nowrap",
        alignItems: "center",
        flexDirection: "row",
        justifyContent: "center"
    },
    tooltipContent: {
        "display": "flex",
        flexDirection: "column",
        flexWrap: "nowrap",
        alignContent: "center",
        justifyContent: "center",
        alignItems: "stretch",
        "background-color": "black",
        "padding": "5px",
        border: "none"
    },
    tooltipHeader: {
        "font-size": "medium",
        "display": "flex",
        flexDirection: "column",
        flexWrap: "nowrap",
        alignContent: "center",
        alignItems: "center",
        justifyContent: "center",
        "border-bottom": "solid 1px"
    },
    threads: {
        "margin": "0px",
        "padding": "0px",
        "display": "flex",
        flexDirection: "column",
        flexWrap: "nowrap",
        alignContent: "center",
        justifyContent: "center",
        alignItems: "stretch",
        "font-size": "small",
    },
    thread: {
        "display": "flex",
        flexDirection: "row",
        flexWrap: "nowrap",
        alignContent: "center",
        justifyContent: "space-between",
        alignItems: "center",
        padding: "0px",
        margin: "0px",
        "column-gap": "5px",
    },
    threadName: {
        padding: "0px",
        margin: "0px"
    },
    threadValue: {
        "display": "flex",
        flexDirection: "row",
        flexWrap: "nowrap",
        alignContent: "center",
        justifyContent: "center",
        alignItems: "center",
        padding: "0px",
        margin: "0px",
        "font-size": "smaller",
        "color": "aquamarine",
        "column-gap": "3px"
    },
    threadSummary: {
        "font-size": "x-small",
        padding: "0px",
        margin: "0px",
        "color": "aqua"
    }
};

export default areaChartStyle;