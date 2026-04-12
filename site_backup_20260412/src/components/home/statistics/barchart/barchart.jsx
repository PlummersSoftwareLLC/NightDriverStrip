import { useTheme, Box, Typography } from "@mui/material";
import barChartStyle from './style'
import  { BarChart, Bar, XAxis, YAxis } from 'recharts'

const BarStat = props => {
    const { name, rawvalue, ignored, statsAnimateChange , idleField, category, detail } = props;
    const theme = useTheme();

    const getFillColor = ({step, isIdle}) => {
        if (isIdle) {
            return theme.palette.taskManager.idleColor;
        }
        return (theme.palette.taskManager[`${category === "Memory" ? "b" : ""}color${step+1}`]);
    };

    const sortStats = (a, b) => {
        return a === idleField && b !== idleField ? 1 : (a !== idleField && b === idleField ? -1 : a.localeCompare(b));
    };

    return (
        <Box sx={barChartStyle.summary}>
            <BarChart
                height={detail ? 300 : 70}
                width={detail ? 150 : 75}
                data={[Object.entries(rawvalue)
                    .filter(entry=>!["name",...ignored].includes(entry[0]))
                    .reduce((ret,entry)=>{ret[entry[0]] = entry[1]; return ret},{name:name})]}>
                <XAxis hide={true} dataKey="name" />
                <YAxis hide={true} />
                {Object.keys(rawvalue)
                    .filter(field => !ignored.includes(field))
                    .sort(sortStats)
                    .map((field,idx) => <Bar dataKey={field} 
                        key={field}
                        stackId="a" 
                        fill={getFillColor({step: idx, isIdle: field === idleField})} 
                        isAnimationActive={statsAnimateChange}
                        type="monotone"
                        fillOpacity={1}/>)
                }
            </BarChart>
            <Typography variant="summary">{(Object.entries(rawvalue)
                .filter(entry => ![idleField,...ignored].includes(entry[0]))
                .reduce((ret,stat)=>ret+stat[1],0.0)/
                                       Object.entries(rawvalue)
                                           .filter(entry => !ignored.includes(entry[0]))
                                           .reduce((ret,stat)=>ret+stat[1],0.0)*100).toFixed(0)}%</Typography>
        </Box>)
};


export default BarStat;