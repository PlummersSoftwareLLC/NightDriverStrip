import { List, ListItem, Typography, Box} from '@mui/material'
import staticStatStyle from './style';

const StaticStatsPanel = props => {
    const {stat, name, detail } = props;

    return <Box sx={staticStatStyle.root}>
        <Typography variant={detail ? "h5" : "h6"}>{name}</Typography>
        {detail ? <List>
            {Object.entries(stat.stat)
                .map(entry=>
                    <ListItem key={entry[0]}>
                        <Typography variant="little" color="textPrimary">{entry[0]}</Typography>:
                        <Typography variant="little" color="textSecondary">{entry[1]}</Typography>
                    </ListItem>)}
        </List>:
            <List>
                {Object.entries(stat.stat)
                    .filter(entry => stat.headerFields.includes(entry[0]))
                    .map(entry=><Typography key={entry[0]} variant="little" color="textSecondary" >{entry[1]}</Typography>)}
            </List>}
    </Box>
};


export default StaticStatsPanel;