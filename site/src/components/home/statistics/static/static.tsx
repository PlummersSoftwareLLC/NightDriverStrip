import { Box, Typography, List, ListItem } from "@mui/material";
import { IStatSpec } from "../../../../models/stats/espstate";
import { withStyles } from "tss-react/mui";
import { staticStatStyle } from "./style"

interface IStatsPanelProps {
    stat: IStatSpec;
    name: string;
    detail: boolean;
    classes?: any;
}

export const StaticStatsPanel = withStyles( ({stat, name, detail, classes }:IStatsPanelProps) => 
    <Box className={classes.root}>
        <Typography >{name}</Typography>
        {detail ? <List>
            {Object.entries(stat.stat)
                   .map(entry=>
                <ListItem key={entry[0]}>
                    <Typography variant="subtitle1" color="textPrimary">{entry[0]}</Typography>:
                    <Typography variant="subtitle2" color="textSecondary">{entry[1]}</Typography>
                </ListItem>)}
        </List>:
        <List>
        {Object.entries(stat.stat)
               .filter(entry => stat.headerFields.includes(entry[0]))
               .map(entry=><ListItem key={entry[0]}><Typography variant="caption" color="textSecondary" >{entry[1]}</Typography></ListItem>)}
        </List>}
    </Box>,staticStatStyle);