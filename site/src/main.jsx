import MainApp from './components/home/home';
import { createRoot } from 'react-dom/client';
import { EffectsProvider } from './context/effectsContext';
import { StatsProvider } from './context/statsContext';

const root = createRoot(document.getElementById('root'));
root.render(
    <StatsProvider>
        <EffectsProvider>
            <MainApp/>
        </EffectsProvider>
    </StatsProvider>
);
