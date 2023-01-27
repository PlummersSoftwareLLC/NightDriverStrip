const drawerWidth = 240;

const mainAppStyle = theme => ({
    root: {
      display: 'flex'
    },
    appbar: {
      zIndex: theme.zIndex.drawer + 1,
      transition: theme.transitions.create(['width', 'margin'], {
        easing: theme.transitions.easing.sharp,
        duration: theme.transitions.duration.leavingScreen,
      }),
    },
    appbarOpened: {
      marginLeft: drawerWidth,
      width: `calc(100% - ${drawerWidth}px)`,
      transition: theme.transitions.create(['width', 'margin'], {
        easing: theme.transitions.easing.sharp,
        duration: theme.transitions.duration.enteringScreen,
      }),
    },
    toolbarTitle: {
      "flex-grow": 1
    },
    drawer: {
      whiteSpace: 'nowrap',
      "z-index": 0,
      width: drawerWidth,
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
      width: theme.spacing.unit * 7,
      [theme.breakpoints.up('sm')]: {
        width: theme.spacing.unit * 9,
      },
    },
    drawerHeader: {
      display: "flex",
      "flex-wrap": "nowrap",
      "min-height": "64px",
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
      flexGrow: 1,
      padding: theme.spacing.unit * 10,
      overflow: 'auto',
      transition: theme.transitions.create('padding-left', {
        easing: theme.transitions.easing.sharp,
        duration: theme.transitions.duration.leavingScreen,
      })
    },
    contentShrinked: {
      "padding-left": drawerWidth + 10,
      transition: theme.transitions.create('padding-left', {
        easing: theme.transitions.easing.sharp,
        duration: theme.transitions.duration.leavingScreen,
      })
    },
    optionSelected: {
      color: "aquamarine"
    }
  });
  