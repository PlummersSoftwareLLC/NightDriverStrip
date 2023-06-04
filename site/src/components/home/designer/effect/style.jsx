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
        borderColor: theme.palette.leds.active.outer,
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
    waiting: {
        borderColor: theme.palette.leds.waiting.outer,
        backgroundColor: theme.palette.leds.waiting.inner,
    },
    lightbar: {
        background: "#fff",
        background: "linear-gradient(90deg, green, blue, red, violet, aquamarine)",
        width: "100%",
        height: "5px",
        borderRadius: "5px",
        opacity: 0.1,
    },
    activelightbar: {
        background: "#fff",
        "-webkit-box-reflect": "below 0px linear-gradient(transparent, white)",
        background: "linear-gradient(9deg, green, blue, red, violet, aquamarine)",
        width: "100%",
        height: "5px",
        borderRadius: "5px"
    },
    effectline: {
        display: "flex",
        flexDirection: "column",
        alignItems: "stretch",
        padding: "0"
    },
    line: {
        display: "flex",
        flexDirection: "row",
        margin: "10px",
        justifyContent: "space-between",
        padding: "5px",
        alignItems: "center"
    },
    effectDetail: {
        display: "flex"
    },
    effectName: {
        display: "flex",
        alignItems: "center",
        columnGap: "10px"
    },
    listButtons: {
        display: "flex",
        flexDirection: "row"
    },
    effectTile: {
        border: "inset 2px",
        borderRadius: "5px",
        borderColor: theme.palette.grey["A700"],
        height: "55px",
    },
    settingicon: {
        zoom: 0.7,
        marginTop: "-60px"
    }
});