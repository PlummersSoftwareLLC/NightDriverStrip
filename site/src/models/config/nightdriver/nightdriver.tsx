export interface INightDriverConfiguration {
	location: string;
	locationIsZip: boolean;
	countryCode: string;
	timeZone: string;
	use24HourClock: boolean;
	useCelsius: boolean;
	ntpServer: string;
	effectInterval: number;
}

export interface INightDriverConfigurationSpecs {
	name: string;
	friendlyName: string;
	description: string;
	type: number;
	typeName: string;
}