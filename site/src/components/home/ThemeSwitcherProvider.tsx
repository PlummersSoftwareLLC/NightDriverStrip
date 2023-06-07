import { Theme, ThemeProvider } from "@mui/material/styles";
import { createContext, useContext, useMemo, useState } from "react";
import { ThemeMode, getTheme } from "../../theme/theme";

// TODO Move out
declare module "@mui/material/styles" {
    interface Palette {
        taskManager: {
            strokeColor: string,
            memoryColor: string,
            idleColor: string,
        }
    }
}

interface IThemeSwitcherContext {
    themeMode: ThemeMode,
    setThemeMode: (mode: ThemeMode) => void,
    theme: Theme
};

export const ThemeSwitcherContext = createContext<IThemeSwitcherContext>(
    {
        themeMode: 'dark',
        setThemeMode: (mode: ThemeMode) => { },
        theme: getTheme('dark')
    }
);

export const useThemeSwitcher = () => {
    if (ThemeSwitcherContext === null) {
        throw new Error('useThemeSwitcher must be used within a ThemeSwitcherProvider');
    }
    const { themeMode, setThemeMode, theme } = useContext(ThemeSwitcherContext);
    return { themeMode, setThemeMode, theme };
};

// There seem to be a bug or something strange behavior with the ThemeProvider from @mui/material/styles that
// makes the useTheme() hook not working properly. It only returns the default theme, not the one from the
// ThemeProvider. So I'm using this custom hook instead.

export function ThemeSwitcherProvider({ children }) {
    const [themeMode, setThemeMode] = useState<ThemeMode>('dark');
    const theme = useMemo(() => getTheme(themeMode), [themeMode]);
    return (
        <ThemeSwitcherContext.Provider value={{ themeMode, setThemeMode, theme }}>
            <ThemeProvider theme={theme}>
                {children}
            </ThemeProvider>
        </ThemeSwitcherContext.Provider>
    );
}