import { Box, Icon, CardContent, Typography, IconButton, Badge, Popover, Card, CardHeader, Avatar, CardActions } from "@mui/material";
import React, { useState, useEffect } from "react";
import { eventManager } from "../../../services/eventManager/eventmanager";

interface INotification {
    date: Date,
    notification: string,
}

interface INotificationGroup {
    level: string;
    type: string;
    target: string;
    notifications: Array<INotification>;
}

export function NotificationPanel() {
    const [service] = useState(eventManager());

    const [notifications, setNotifications] = React.useState([] as INotificationGroup[]);
    const [numErrors, setNumErrors] = React.useState(undefined as unknown as number);
    const [errorTargets, setErrorTargets] = React.useState({});
    const [open, setOpen] = React.useState(false);

    useEffect(()=>{
        setNumErrors(notifications.reduce((ret,notif) => ret+notif.notifications.length, 0));
        setErrorTargets(notifications.reduce((ret,notif) => {return {...ret,[notif.target]:ret[notif.target] || false}}, {}));
    },[notifications]);

    useEffect(()=>{
        service.subscribe("Error",props => {
            const {level,type,target,notification} = props;
            setNotifications(prevNotifs => {
                const group = prevNotifs.find(notif=>(notif.level === level) && (notif.type == type) && (notif.target === target)) ?? {level,type,target,notifications:new Array<INotification>()};
                group.notifications.push({date:new Date(),notification} as INotification);
                return [...prevNotifs.filter(notif => notif !== group), group];
            })});
    },[service]);

    return (
        (notifications.length > 0) ? <Box>
            <IconButton
                    id="notifications"
                    onClick={() => setOpen(wasOpen=>!wasOpen)}>
                <Badge 
                    aria-label="Alerts" 
                    badgeContent={numErrors}>
                    <Icon>notifications</Icon>
                </Badge>
            </IconButton>
            <Popover
                open={open}
                onClose={()=>{setOpen(false)}}
                anchorOrigin={{
                    vertical: 'top',
                    horizontal: 'right',
                }}>
                <Card>
                    <CardHeader
                        avatar={
                        <Avatar aria-label="error">
                            !
                        </Avatar>
                        }
                        action={
                        <IconButton aria-label="Close" onClick={()=>setOpen(false)}>
                            <Icon>close</Icon>
                        </IconButton>
                        }
                        title={`${numErrors} Errors`}
                    />
                    <CardContent>
                        {Object.entries(errorTargets)
                               .sort((a,b)=>a[0].localeCompare(b[0]))
                               .map(target => 
                            <CardContent key={target[0]}>
                                {Object.entries(notifications)
                                       .filter(notif => notif[1].target === target[0])
                                       .map(error =>
                                <Box key={error[0]}>
                                    <Box key="header">
                                        <Typography>{`${target[0]}`}</Typography>
                                        <Typography color="textSecondary">{error[1].type}</Typography>
                                        <Typography>{error[1].level}</Typography>
                                    </Box>
                                    <Box key="errors">
                                        {Object.entries(error[1].notifications.reduce((ret,error) => {return {...ret,[error.notification]:(ret[error.notification]||0)+1}},{}))
                                                .map(entry => <Typography key={entry[1] as string} variant="subtitle1">{`${entry[1]}X ${entry[0]}`}</Typography>)
                                        }
                                    </Box>
                                </Box>
                                       )}
                            </CardContent>
                        )}
                    </CardContent>
                    <CardActions disableSpacing>
                        <IconButton onClick={()=>setNotifications([])} aria-label="Clear Errors">
                            <Icon>delete</Icon>
                        </IconButton>
                    </CardActions>
                </Card>
            </Popover>
        </Box>:<div></div>);
}