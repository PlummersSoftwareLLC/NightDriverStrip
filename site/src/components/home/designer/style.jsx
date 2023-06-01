const designStyle = theme => ({
    root: {
        "display": "flex",
        "flex-direction": "column",
    },
    hidden: {
        display: "none"
    },
    effectsHeader: {
        display: "flex",
        flexDirection: "row",
        borderBottom: "solid 1px",
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
        padding: "10px",
        columnGap: "10px",
        rowGap: "10px",
        paddingTop: "20px",
    }
});