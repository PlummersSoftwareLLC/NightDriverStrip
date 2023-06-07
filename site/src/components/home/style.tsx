export const mainAppStyle = (theme,{openDrawerWidth,closeDrawerWidth,toolbarHeight}) => ({
    root: {
      display: 'flex'
    },
    appbar: {
      zIndex: theme.zIndex.drawer + 1,
      minHeight: toolbarHeight,
      transition: theme.transitions.create(['width', 'margin'], {
        easing: theme.transitions.easing.sharp,
        duration: theme.transitions.duration.leavingScreen,
      }),
    },
    appbarOpened: {
      marginLeft: openDrawerWidth,
      width: `calc(100% - ${openDrawerWidth}px)`,
      minHeight: toolbarHeight,
      transition: theme.transitions.create(['width', 'margin'], {
        easing: theme.transitions.easing.sharp,
        duration: theme.transitions.duration.enteringScreen,
      }),
    },
    categoryStats: {
        "display": "flex",
        "flex-direction": "row",
        "flex-wrap": "wrap",
        "align-content": "flex-start",
        "justify-content": "center",
        "align-items": "center"
    },
    toolbarTitle: {
      "flex-grow": "1"
    },
    drawer: {
      whiteSpace: 'nowrap',
      "z-index": 0,
      width: openDrawerWidth,
      transition: theme.transitions.create('width', {
        easing: theme.transitions.easing.sharp,
        duration: theme.transitions.duration.enteringScreen,
      }),
    },
    drawerClosed: {
      overflowX: 'hidden',
      transition: theme.transitions.create('width', {
        easing: theme.transitions.easing.sharp,
        duration: theme.transitions.duration.leavingScreen,
      }),
      width: closeDrawerWidth,
      [theme.breakpoints.up('sm')]: {
        width: theme.spacing.unit * 9,
      },
    },
    drawerHeader: {
      display: "flex",
      "flex-wrap": "nowrap",
      "min-height": toolbarHeight,
      "flex-direction": "row",
      "justify-content": "space-between"
    },
    displayMode: {
      display: "flex",
      "flex-wrap": "nowrap",
      "flex-direction": "row",
      "justify-content": "flex-start",
      "align-items": "center"
    },
    content: {
      padding: `64px 0 0 ${openDrawerWidth}px`,
      transition: theme.transitions.create('padding-left', {
        easing: theme.transitions.easing.sharp,
        duration: theme.transitions.duration.leavingScreen,
      }),
      display: "flex",
      flexDirection: "column",
      flexWrap: "wrap",
      rowGap: "0px",
      alignItems: "stretch",
      width: "100%",
    },
    contentShrinked: {
      "padding-left": closeDrawerWidth + 10,
      rowGap: "10px",
      transition: theme.transitions.create('padding-left', {
        easing: theme.transitions.easing.sharp,
        duration: theme.transitions.duration.leavingScreen,
      })
    },
    optionSelected: {
      color: theme.palette.primary.contrastText
    },
    optionUnselected: {
      color: theme.palette.primary.main
    },
    setting: {
      display: "flex",
      flexDirection: "row",
      flexWrap: "wrap",
    },
    settingItem: {
      width: "fit-content"
    }
  });
  