export interface DefaultCssVarsTheme {
    colorSchemes: Record<string, any>;
}
declare function prepareCssVars<T extends DefaultCssVarsTheme, ThemeVars extends Record<string, any>>(theme: T, parserConfig?: {
    prefix?: string;
    shouldSkipGeneratingVar?: (objectPathKeys: Array<string>, value: string | number) => boolean;
}): {
    vars: ThemeVars;
    generateCssVars: (colorScheme?: string) => {
        css: {
            [x: string]: string | number;
        };
        vars: ThemeVars;
    };
};
export default prepareCssVars;
