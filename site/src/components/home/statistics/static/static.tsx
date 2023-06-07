import { Box, Typography, List, ListItem } from "@mui/material";
import { IStatSpec } from "../../../../models/stats/espstate";

interface IStatsPanelProps {
    stat: IStatSpec;
    name: string;
    detail: boolean;
}

export function StaticStatsPanel({stat, name, detail }:IStatsPanelProps) {

    return <Box>
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
    </Box>
};