import MainApp from './components/home/home';
import { createRoot } from 'react-dom/client';
import { EffectsProvider } from './context/effectsContext';

const root = createRoot(document.getElementById('root'));
root.render(
    <EffectsProvider>
        <MainApp/>
    </EffectsProvider>
);
