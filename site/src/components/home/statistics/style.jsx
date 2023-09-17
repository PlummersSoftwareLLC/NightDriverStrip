const statsStyle = {
    root: {
        display: "flex",
        flexDirection: "row",
        flexWrap: "wrap",
        alignContent: "flex-start",
        justifyContent: "flex-start",
        alignItems: "stretch",
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
        display: "flex",
        flexDirection: "row",
        flexWrap: "wrap",
        alignContent: "flex-start",
        justifyContent: "flex-start",
        alignItems: "flex-start"
    },
    chartHeader: {
        display: "flex",
        flexDirection: "column",
        flexWrap: "wrap",
        alignContent: "center",
        justifyContent: "flex-start",
        alignItems: "flex-start"
    },
    categoryStats:{
        display: "flex",
        flexDirection: "row",
        flexWrap: "wrap",
        alignContent: "flex-start",
        justifyContent: "space-between",
        alignItems: "flex-start",
        columnGap: "5px",
    },
    category:{
    },
    hidden: {
        display: "none"
    },
    statCatergoryHeader: {
        display: "flex",
        flexDirection: "row",
        flexWrap: "wrap",
        alignContent: "flex-start",
        justifyContent: "space-between",
        alignItems: "center",
        borderBottom: "solid 1px",
    }
};

const statStyle = () => ({
    root: {
        display: 'flex',
        flexDirection: "column"
    }
});

export {statStyle, statsStyle}