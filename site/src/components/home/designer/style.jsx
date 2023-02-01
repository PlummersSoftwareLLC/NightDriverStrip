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
    effects: {
        display: "flex",
        flexDirection: "row",
        flexWrap: "wrap",
    }
});