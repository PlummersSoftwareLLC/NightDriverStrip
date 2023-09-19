const effectStyle = (theme) => {
    return {
        root: {
            display: "flex",
            flexDirection: "column",
            padding: "10px",
        },
        cardheader: {
        },
        hidden: {
            display: "none"
        },
        effect: {
            width: "180px",
            display: "flex",
            flexDirection: "column",
            flexWrap: "nowrap",
            justifyContent: "space-between",
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
    }
};

export default effectStyle;