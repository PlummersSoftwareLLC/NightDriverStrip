const chartStyle = theme => ({
    root: {
        "display": "flex",
        "flex-direction": "column",
        "flex-wrap": "wrap",
        "align-content": "flex-start",
        "justify-content": "flex-start",
        "align-items": "stretch"
    },
    header: {
        "display": "flex",
        "flex-direction": "column",
        "flex-wrap": "nowrap",
        "justify-content": "center",
        "align-items": "stretch"
    },
    stats: {
        "display": "flex",
        "flex-direction": "row",
        "flex-wrap": "nowrap",
        "justify-content": "flex-start",
        "align-items": "center",
        "padding": "0px"
    },
    stat: {
        "display": "flex",
        "flex-direction": "row",
        "flex-wrap": "nowrap",
        "justify-content": "flex-start",
        "align-items": "center",
        "padding": "0px",
        "column-gap": "3px"
    },
    tooltipContent: {
        "display": "flex",
        "flex-direction": "column",
        "flex-wrap": "nowrap",
        "align-content": "center",
        "justify-content": "center",
        "align-items": "stretch",
        "background-color": "black",
        "padding": "5px"
    },
    tooltipHeader: {
        "font-size": "medium",
        "display": "flex",
        "flex-direction": "column",
        "flex-wrap": "nowrap",
        "align-content": "center",
        "align-items": "center",
        "justify-content": "center",
        "border-bottom": "solid 1px"
    },
    threads: {
        "margin": "0px",
        "padding": "0px",
        "display": "flex",
        "flex-direction": "column",
        "flex-wrap": "nowrap",
        "align-content": "center",
        "justify-content": "center",
        "align-items": "stretch",
        "font-size": "small",
    },
    thread: {
        "display": "flex",
        "flex-direction": "row",
        "flex-wrap": "nowrap",
        "align-content": "center",
        "justify-content": "space-between",
        "align-items": "center",
        "column-gap": "5px",
    },
    threadValue: {
        "display": "flex",
        "flex-direction": "row",
        "flex-wrap": "nowrap",
        "align-content": "center",
        "justify-content": "center",
        "align-items": "center",
        "font-size": "smaller",
        "color": "aquamarine",
        "column-gap": "3px"
    },
    threadSummary: {
        "font-size": "x-small",
        "color": "aqua"
    }
});
