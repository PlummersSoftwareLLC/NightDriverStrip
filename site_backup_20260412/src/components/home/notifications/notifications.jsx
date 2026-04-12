
import notificationsStyle from "./style";
import { useState, createRef, useEffect } from "react";
import { Box, IconButton, Badge, Icon, Popover, Card,CardHeader, Avatar } from "@mui/material";
import {useTheme, CardContent, Typography, CardActions } from "@mui/material";

const NotificationPanel = props => {
    const { notifications, clearNotifications } = props;
    const [numErrors, setNumErrors] = useState(undefined);
    const [errorTargets, setErrorTargets] = useState({});
    const [open, setOpen] = useState(false);
    const inputRef = createRef();
    const theme = useTheme();

    useEffect(()=>{
        setNumErrors(notifications.reduce((ret,notif) => ret+notif.notifications.length, 0));
        setErrorTargets(notifications.reduce((ret,notif) => 
        {return {...ret,[notif.target]:ret[notif.target] || false}}, {}));
    },[notifications]);

    return (
        <Box sx={notificationsStyle.root}>
            <IconButton
                id="notifications"
                ref={inputRef}
                onClick={() => setOpen(wasOpen=>!wasOpen)}>
                <Badge 
                    aria-label="Alerts" 
                    badgeContent={numErrors} 
                    color="secondary">
                    <Icon>notifications</Icon>
                </Badge>
            </IconButton>
            <Popover
                open={open}
                target="notifications"
                onClose={()=>{setOpen(false)}}
                anchorOrigin={{
                    vertical: 'top',
                    horizontal: 'right',
                }}>
                <Card sx={notificationsStyle.popup} elevation={9}>
                    <CardHeader
                        avatar={
                            <Avatar sx={{ bgcolor: theme.palette.error.dark }} aria-label="error">
                            !
                            </Avatar>
                        }
                        action={
                            <IconButton onClick={()=>setOpen(false)} aria-label="settings">
                                <Icon>close</Icon>
                            </IconButton>
                        }
                        title={`${numErrors} Errors`}
                    />
                    <CardContent>
                        {Object.entries(errorTargets)
                            .sort((a,b)=>a[0].localeCompare(b[0]))
                            .map(target => 
                                <CardContent key={target[0]} sx={notificationsStyle.errors}>
                                    {Object.entries(notifications)
                                        .filter(notif => notif[1].target === target[0])
                                        .map(error =>
                                            <Box key={error[0]}>
                                                <Box sx={notificationsStyle.errorHeader} key="header">
                                                    <Typography>{target[0]}</Typography>
                                                    <Typography color="textSecondary">{error[1].type}</Typography>
                                                    <Typography>{error[1].level}</Typography>
                                                </Box>
                                                <Box sx={notificationsStyle.errors} key="errors">
                                                    {Object.entries(error[1].notifications.reduce((ret,error) => {return {...ret,[error.notification]:(ret[error.notification]||0)+1}},{}))
                                                        .map(entry => <Typography key={entry[1]} variant="tiny">{`${entry[1]}X ${entry[0]}`}</Typography>)
                                                    }
                                                </Box>
                                            </Box>
                                        )}
                                </CardContent>
                            )}
                    </CardContent>
                    <CardActions disableSpacing>
                        <IconButton onClick={()=>clearNotifications()} aria-label="Clear Errors">
                            <Icon>delete</Icon>
                        </IconButton>
                    </CardActions>
                </Card>
            </Popover>
        </Box>);
};

export default NotificationPanel;