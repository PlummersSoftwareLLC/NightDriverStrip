const designStyle = theme => ({
    root: {
        "display": "flex",
        "flex-direction": "column",
        border: "green solid 2px",
        borderRadius: "15px",
        padding: "10px",
    },
    hidden: {
        display: "none"
    },
    effectsHeader: {
        display: "flex",
        flexDirection: "row",
        borderBottom: "green solid 1px",
        columnGap: "5px",
    },
    effectsHeaderValue: {
        display: "flex",
        flexDirection: "row",
        columnGap: "3px",
        alignItems: "center",
    },
    effect: {
        display: "flex",
        flexDirection: "row",
        alignItems: "center",
        width: "180px",
        border: "green solid 1px"
    },
    effects: {
        display: "flex",
        flexDirection: "row",
        flexWrap: "wrap",
    },
    timeremaining: {
        width: "50px"
    },
    effectAttribute: {
        display: "flex",
        flexDirection: "row",
        alignItems: "center",
        width: "100%",
    },
    unselected: {
        opacity: "30%"
    },
    selected: {
        backgroundColor: theme.palette.background.paper,
    }
});