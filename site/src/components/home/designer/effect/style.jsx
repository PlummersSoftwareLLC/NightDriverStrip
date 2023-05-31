const effectStyle = theme => ({
    root: {
        "display": "flex",
        "flex-direction": "column",
        padding: "10px",
    },
    hidden: {
        display: "none"
    },
    effect: {
        width: "180px",
        height: "fit-content",
        display: "flex",
        flexDirection: "column",
        flexWrap: "nowrap",
        justifyContent: "space-between",
        alignItems: "center",
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
    },
    circularProgressText: {
        position: "relative",
        left: "-29px",
        top:"-15px"    
    },
    disabled: {
        opacity: "50%"
    },
    cardcontent: {
        paddingTop: "5px",
        paddingBottom: "5px"
    },
    cardactions: {
        paddingTop: "5px",
        paddingBottom: "5px"
    }
});