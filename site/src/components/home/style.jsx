const drawerWidth = 240;

const mainAppStyle = (theme) => ({ 
    root: {
        display: 'flex'
    },
    appbar: {
        transition: "width 0.2s linear"
    },
    appbarClosed: {
        zIndex: "1300 !important",
    },
    appbarOpened: {
        marginLeft: drawerWidth,
        width: `calc(100% - ${drawerWidth}px)`,
    },
    toolbarTitle: {
        flexGrow: 1
    },
    drawer: {
        whiteSpace: 'nowrap',
        width: drawerWidth,
        transition: "width 0.2s linear",
    },
    drawerClosed: {
        overflowX: 'hidden',
        transition: "width 0.2s linear",
        width: theme.spacing.unit * 7,
        maxWidth: "65px",
        [theme.breakpoints.up('sm')]: {
            width: theme.spacing.unit * 9,
        },
    },
    drawerHeader: {
        display: "flex",
        flexWrap: "nowrap",
        minHeight: "64px",
        flexDirection: "row",
        justifyContent: "space-between"
    },
    displayMode: {
        display: "flex",
        flexWrap: "nowrap",
        flexDirection: "row",
        justifyContent: "flex-start",
        alignItems: "center"
    },
    content: {
        transition: "padding-left 0.2s linear",
    },
});
  
export default mainAppStyle;