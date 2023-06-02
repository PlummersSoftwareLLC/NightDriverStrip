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
    },
    playing: {
        borderColor: theme.palette.primary.contrastText,
        borderWidth: "3px",
    },
    dot: {
        height: "10px",
        width: "10px",
        borderRadius: "50%",
        display: "inline-block",
        borderStyle: "groove",
        cursor: "pointer",
    },
    selected: {
        borderColor: theme.palette.leds.active.outer,
        backgroundColor: theme.palette.leds.active.inner,
    },
    waiting: {
        borderColor: theme.palette.leds.waiting.outer,
        backgroundColor: theme.palette.leds.waiting.inner,
    },
});