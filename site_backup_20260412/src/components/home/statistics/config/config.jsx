import { List } from "@mui/material";
import ConfigItem from "./configItem";
import PropTypes from 'prop-types'

const ConfigPanel = props => {
    const { siteConfig } = props;
    return (
        <List>
            {Object.entries(siteConfig).map(entry => <ConfigItem 
                name={entry[1].name}
                key={entry[1].name}
                datatype={entry[1].type}
                value={entry[1].value}
                configItemUpdated={value => entry[1].setter(value)} 
            />)}
        </List>
    );
};

ConfigPanel.propTypes = {
    siteConfig: PropTypes.shape({
    }).isRequired,
}

export default ConfigPanel;