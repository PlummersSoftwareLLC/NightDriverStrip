const effectStyle = theme => ({
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
    effect: {
        display: "flex",
        flexDirection: "column",
        alignItems: "stretch",
        width: "180px",
        border: "green solid 1px"
    },
    effectPannel: {
        display: "flex",
        flexDirection: "row",
        alignItems: "center",
        justifyContent: "space-around",
    },
    effectName: {
        marginLeft: "10px",
        marginBottom: "5px"
    },
    unselected: {
        opacity: "30%"
    },
    selected: {
        backgroundColor: theme.palette.background.paper,
    }
});