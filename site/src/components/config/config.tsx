import { List, ListItem, ListItemText, Typography, Divider } from "@mui/material";
import { useState, useEffect } from "react";
import { eventManager } from "../../services/eventManager/eventmanager";
import { ChipConfigItem } from "./chipconfigItem";
import { SiteConfigItem } from "./siteconfigItem";
import { ISiteConfig } from "../../models/config/site/siteconfig";
import { INightDriverConfiguration, INightDriverConfigurationSpecs } from "../../models/config/nightdriver/nightdriver";
import { withStyles } from 'tss-react/mui';
import { configStyle } from "./style";

interface IConfigPanelProps {
  classes?: any;
}

export const ConfigPanel = withStyles(({classes}:IConfigPanelProps) => {
  const [siteConfig, setSiteConfig] = useState(undefined as unknown as ISiteConfig);
  const [chipConfig, setChipConfig] = useState(undefined as unknown as INightDriverConfiguration);
  const [chipConfigSpec, setChipConfigSpec] = useState(undefined as unknown as [INightDriverConfigurationSpecs]);
  const [service] = useState(eventManager());

  useEffect(()=>{
    const subs = {
      siteConfig: service.subscribe("SiteConfig",cfg=>{setSiteConfig(cfg)}),
      chipConfig: service.subscribe("ChipConfig",cfg=>{setChipConfig(cfg)}),
      chipConfigSpec: service.subscribe("ChipConfigSpec",cfg=>{setChipConfigSpec(cfg)})
    }
    return ()=>{Object.values(subs).forEach(service.unsubscribe)};
  },[service]);

  if (siteConfig === undefined) {
    return <div>Loading...</div>
  }

  return (
    <List className={classes.configsection}>
      <ListItem className={classes.configsection}>
        <ListItemText><Typography variant="overline" color="textSecondary">NightDriver</Typography></ListItemText>
        <Divider />
        {chipConfig&&chipConfigSpec?Object.entries(chipConfig).map(entry => <ChipConfigItem 
                                        key={entry[0]}
                                        id={entry[0]}
                                        value={entry[1]}
                                        {...chipConfigSpec.find(cs=>cs.name===entry[0]) as INightDriverConfigurationSpecs}/>):<div>Loading...</div>}
      </ListItem>
      <ListItem className={classes.configsection}>
        <ListItemText><Typography variant="overline" color="textSecondary">Site</Typography></ListItemText>
        <Divider />
        {siteConfig?Object.entries(siteConfig).map(entry => <SiteConfigItem 
                                                                key={entry[0]}
                                                                id={entry[0]}
                                                                typeName={entry[1].type}
                                                                {...entry[1]}/>):<div>Loading...</div>}
      </ListItem>
    </List>
  );
},configStyle);