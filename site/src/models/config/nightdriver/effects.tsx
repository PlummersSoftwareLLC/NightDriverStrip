export interface IEffect {
	name: string;
	enabled: boolean;
	core: boolean;
}

export interface IEffects {
	currentEffect: number;
	millisecondsRemaining: number;
	effectInterval: number;
	Effects: IEffect[];
}