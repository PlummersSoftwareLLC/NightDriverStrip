import { AppPannel } from "./AppPannel";
import { CssBaseline } from "@mui/material";
import { ThemeSwitcherProvider } from "./ThemeSwitcherProvider";
import { ScreenService } from '../../services/screen/screenmanager';

const screenService = ScreenService();
export function MainApp() {
    return (<ThemeSwitcherProvider>
        <CssBaseline />
        <AppPannel />
    </ThemeSwitcherProvider>);
}