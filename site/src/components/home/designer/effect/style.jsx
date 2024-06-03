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
        gridCard: {
            width: "180px",
            display: "flex",
            flexDirection: "column",
            flexWrap: "nowrap",
            justifyContent: "space-between",
        },
        listCard: {
            ":after": {content: '""', display: "table", clear: "both"}, 
            width: "100%",
            height: "50px" 
        },
        effectName: {
            marginLeft: "10px",
            marginBottom: "5px"
        },
        unselected: {
            opacity: "30%"
        },
        short: {
            paddingTop: "2px",
            paddingBottom: "2px"
        },
        selected: {
            backgroundColor: theme.palette.background.paper,
        }
    }
};

export default effectStyle;