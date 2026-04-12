import { createRoot } from 'react-dom/client';
import MainApp from './components/home/home';
import { EffectsProvider } from './context/effectsContext';
import { StatsProvider } from './context/statsContext';
import './styles.css';

const savedTheme = localStorage.getItem('theme') || 'dark';
document.documentElement.setAttribute('data-theme', savedTheme);

const root = createRoot(document.getElementById('root'));
root.render(
    <StatsProvider>
        <EffectsProvider>
            <MainApp/>
        </EffectsProvider>
    </StatsProvider>
);
