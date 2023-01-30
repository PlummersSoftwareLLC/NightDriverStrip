const statsStyle = theme => ({
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
        border: "green solid 2px",
        borderRadius: "15px",
    },
    hidden: {
        display: "none"
    },
    statCatergoryHeader: {
        "display": "flex",
        "flex-direction": "row",
        "flex-wrap": "wrap",
        "align-content": "flex-start",
        "justify-content": "center",
        "align-items": "center",
        borderBottom: "green solid 1px"
    }
});

const statStyle = theme => ({
    root: {
      display: 'flex',
      "flex-direction": "column"
    }
});
