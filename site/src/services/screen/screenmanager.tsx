import { eventManager } from "../eventManager/eventmanager";

export const ScreenService = () => {
    const mediaQueries = {
        isDesktopOrLaptop : window.matchMedia('(min-width: 1224px)'),
        isBigScreen : window.matchMedia('(min-width: 1824px)' ),
        isTabletOrMobile : window.matchMedia('(max-width: 1224px)' ),
        isLandscape : window.matchMedia('(orientation:landscape)' ),
        isPortrait : window.matchMedia('(orientation:portrait)' ),
        isRetina : window.matchMedia('(min-resolution: 2dppx)' ),
    };

    const isSmallScreen = () => {
        return mediaQueries.isTabletOrMobile.matches;
    };
    const service = eventManager();

    Object.values(mediaQueries)
        .every(mediaQuery=> mediaQuery.addEventListener("change",()=>service.emit("isSmallScreen",isSmallScreen())));

    return service.subscribe("subscription",sub=>service.emit("isSmallScreen",isSmallScreen(),sub));
};
