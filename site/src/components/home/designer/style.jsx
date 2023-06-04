const designStyle = theme => ({
    root: {
        "display": "flex",
        "flex-direction": "column",
    },
    summaryRoot: {
        display: "flex",
        flexDirection: "column",
        width: "130px",
        border: "solid 2px",
        borderRadius: "10px",
        padding: "5px",
    },
    hidden: {
        display: "none"
    },
    shownSome: {
        width: "max-content",
    },
    effectsHeader: {
        display: "flex",
        flexDirection: "row",
        borderBottom: "solid 1px",
        columnGap: "5px",
        justifyContent: "space-between",
    },
    controls: {
        display: "flex",
        flexDirection: "row",
        flexWrap: "nowrap",
        justifyContent: "flex-start",
        alignItems: "center"
    },
    effectsHeaderValue: {
        display: "flex",
        flexDirection: "row",
        columnGap: "3px",
        alignItems: "center",
    },
    effects: {
        display: "flex",
        flexDirection: "column",
        padding: "0"
    },
    summaryEffects: {
        display: "flex",
        padding: "10px",
        rowGap: "1px",
        flexWrap: "wrap",
        columnGap: "1px",
        flexDirection: "row",
        alignContent: "flex-start",
        justifyContent: "flex-start",
        alignItems: "flex-start",
        width: "130px",
    },
    cardHeader: {
        display: "flex",
        flexDirection: "row",
        flexWrap: "nowrap",
        justifyContent: "space-between",
    },
    progress: {
        '&[aria-valuenow]': {
            "& > .MuiLinearProgress-bar1Determinate": {
              transition: ".1s"
            }
          }
    }
});