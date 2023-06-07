export interface ISiteConfig {
  [key: string]: {
    name: string;
    value: any;
    type: string;
  };
}

export interface IEffectOption {
	name: string;
	typeName: string;
	value: string;
}

export interface IEffectSettings {
	[key:string]: IEffectOption;
}