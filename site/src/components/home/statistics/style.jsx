const statsStyle = () => ({
    root: {
        "display": "flex",
        "flex-direction": "row",
        "flex-wrap": "wrap",
        "align-content": "flex-start",
        "justify-content": "flex-start",
        "align-items": "stretch",
        columnGap: "10px",
        rowGap: "10px"
    },
    summaryStats: {
        cursor: "pointer"
    },
    detiailedStats: {
        border: "solid 1px"
    },
    chartArea: {
        "display": "flex",
        "flex-direction": "row",
        "flex-wrap": "wrap",
        "align-content": "flex-start",
        "justify-content": "flex-start",
        "align-items": "flex-start"
    },
    chartHeader: {
        "display": "flex",
        "flex-direction": "column",
        "flex-wrap": "wrap",
        "align-content": "center",
        "justify-content": "flex-start",
        "align-items": "flex-start"
    },
    categoryStats:{
        "display": "flex",
        "flex-direction": "row",
        "flex-wrap": "wrap",
        "align-content": "flex-start",
        "justify-content": "space-between",
        "align-items": "flex-start",
        "column-gap": "5px",
    },
    category:{
    },
    hidden: {
        display: "none"
    },
    statCatergoryHeader: {
        "display": "flex",
        "flex-direction": "row",
        "flex-wrap": "wrap",
        "align-content": "flex-start",
        "justify-content": "space-between",
        "align-items": "center",
        "border-bottom": "solid 1px",
    }
});

const statStyle = () => ({
    root: {
      display: 'flex',
      "flex-direction": "column"
    }
});

export {statStyle, statsStyle}