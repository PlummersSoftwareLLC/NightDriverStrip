export interface IESPState {
	LED_FPS: number;
	SERIAL_FPS: number;
	AUDIO_FPS: number;
	HEAP_SIZE: number;
	HEAP_FREE: number;
	HEAP_MIN: number;
	DMA_SIZE: number;
	DMA_FREE: number;
	DMA_MIN: number;
	PSRAM_SIZE: number;
	PSRAM_FREE: number;
	PSRAM_MIN: number;
	CHIP_MODEL: string;
	CHIP_CORES: number;
	CHIP_SPEED: number;
	PROG_SIZE: number;
	CODE_SIZE: number;
	CODE_FREE: number;
	FLASH_SIZE: number;
	CPU_USED: number;
	CPU_USED_CORE0: number;
	CPU_USED_CORE1: number;
}

export interface ICPUStat {
	CORE0: number;
	CORE1: number;
	IDLE: number;
	USED: number;
}

export interface ICPU {
	CPU: IStatSpec;
}

export interface IMemoryStat {
	USED: number;
	FREE: number;
	MIN: number;
	SIZE: number;
}

export interface IStatSpec {
	stat: IMemoryStat|ICPUStat;
	idleField: string;
	headerFields: string[];
	ignored: string[];
}

export interface IMemory {
	HEAP: IStatSpec;
	DMA: IStatSpec;
	PSRAM: IStatSpec;
}

export interface INightDriverStat {
	LED: number;
	SERIAL: number;
	AUDIO: number;
}

export interface IFP {
	stat: INightDriverStat;
}

export interface INightDriver {
	FPS: IFP;
}

export interface IChipStat {
	MODEL: string;
	CORES: number;
	SPEED: number;
	PROG_SIZE: number;
}

export interface ICHIP {
	stat: IChipStat;
	static: boolean;
	headerFields: string[];
}

export interface ICodeStat {
	SIZE: number;
	FREE: number;
	FLASH_SIZE: number;
}

export interface ICODE {
	stat: ICodeStat;
	static: boolean;
	headerFields: string[];
}

export interface IPackage {
	CHIP: ICHIP;
	CODE: ICODE;
}

export interface IConvertedStat {
	CPU: ICPU;
	Memory: IMemory;
	NightDriver: INightDriver;
	Package: IPackage;
}