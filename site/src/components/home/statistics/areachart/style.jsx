const areaChartStyle = {
    root: {
        display: "flex",
        flexDirection: "column",
        flexWrap: "wrap",
        alignContent: "flex-start",
        justifyContent: "flex-start",
        alignItems: "stretch"
    },
    header: {
        display: "flex",
        flexDirection: "column",
        flexWrap: "nowrap",
        justifyContent: "center",
        alignItems: "stretch"
    },
    headerLine: {
        display: "flex",
        flexDirection: "row",
        flexWrap: "nowrap",
        justifyContent: "space-between",
        alignItems: "center",
        width: "100%"
    },
    headerField: {
        display: "flex",
        flexDirection: "row",
        flexWrap: "nowrap",
        justifyContent: "center",
        alignItems: "center",
        width: "inherit"
    },
    stats: {
        display: "flex",
        flexDirection: "row",
        flexWrap: "nowrap",
        justifyContent: "center",
        alignItems: "center",
        padding: "0px"
    },
    stat: {
        display: "flex",
        padding: "0px",
        flexWrap: "nowrap",
        alignItems: "center",
        flexDirection: "row",
        justifyContent: "center"
    },
    tooltipContent: {
        display: "flex",
        flexDirection: "column",
        flexWrap: "nowrap",
        alignContent: "center",
        justifyContent: "center",
        alignItems: "stretch",
        backgroundColor: "black",
        padding: "5px",
        border: "none"
    },
    tooltipHeader: {
        fontSize: "medium",
        display: "flex",
        flexDirection: "column",
        flexWrap: "nowrap",
        alignContent: "center",
        alignItems: "center",
        justifyContent: "center",
        borderBottom: "solid 1px"
    },
    threads: {
        margin: "0px",
        padding: "0px",
        display: "flex",
        flexDirection: "column",
        flexWrap: "nowrap",
        alignContent: "center",
        justifyContent: "center",
        alignItems: "stretch",
        border: "none",
        fontSize: "small",
    },
    thread: {
        display: "flex",
        flexDirection: "row",
        flexWrap: "nowrap",
        alignContent: "center",
        justifyContent: "space-between",
        alignItems: "center",
        padding: "0px",
        margin: "0px",
        columnGap: "5px",
    },
    threadName: {
        padding: "0px",
        margin: "0px",
        border: "none"
    },
    threadValue: {
        display: "flex",
        flexDirection: "row",
        flexWrap: "nowrap",
        alignContent: "center",
        justifyContent: "center",
        alignItems: "center",
        padding: "0px",
        margin: "0px",
        fontSize: "smaller",
        color: "aquamarine",
        border: "none",
        columnGap: "3px"
    },
    threadSummary: {
        fontSize: "x-small",
        padding: "0px",
        margin: "0px",
        color: "aqua"
    }
};

export default areaChartStyle;