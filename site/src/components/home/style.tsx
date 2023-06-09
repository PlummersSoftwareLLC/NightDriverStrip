import {DrawerWidth} from "./AppPannel"

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
      marginLeft: `${openDrawerWidth}px`,
      width: `calc(100% - ${openDrawerWidth}px)`,
      minHeight: toolbarHeight,
      transition: theme.transitions.create(['width', 'margin'], {
        easing: theme.transitions.easing.sharp,
        duration: theme.transitions.duration.enteringScreen,
      }),
    },
    categoryStats: {
        display: "flex",
        flexDirection: "row",
        flexWrap: "wrap",
        alignContent: "flexStart",
        justifyContent: "center",
        alignItems: "center"
    },
    toolbarTitle: {
      flexGrow: "1"
    },
    drawer: {
      whiteSpace: 'nowrap',
      zIndex: 0,
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
      flexWrap: "nowrap",
      minHeight: toolbarHeight,
      flexDirection: "row",
      justifyContent: "space-between"
    },
    displayMode: {
      display: "flex",
      flexWrap: "nowrap",
      flexDirection: "row",
      justifyContent: "flexStart",
      alignItems: "center"
    },
    content: {
      paddingLeft: `${DrawerWidth.close + 10}px`,
      paddingTop: `${toolbarHeight}px`,
      transition: theme.transitions.create('padding-left', {
          easing: theme.transitions.easing.sharp,
          duration: theme.transitions.duration.leavingScreen,
      }),
      display: "flex",
      flexDirection: "column",
      flexWrap: "wrap",
      rowGap: "0px",
    },
    contentShrinked: {
      paddingLeft:  `${DrawerWidth.open + 25}px`,
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
      width: "fitContent"
    }
  });
  