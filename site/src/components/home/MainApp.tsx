import { AppPannel } from "./AppPannel";
import { CssBaseline } from "@mui/material";
import { ThemeSwitcherProvider } from "./ThemeSwitcherProvider";

export function MainApp() {
    return (<ThemeSwitcherProvider>
        <CssBaseline />
        <AppPannel />
    </ThemeSwitcherProvider>);
}