import MainApp from './components/home/home';
import { createRoot } from 'react-dom/client';
import { TimingProvider } from './context/effectContext';

const root = createRoot(document.getElementById('root'));
root.render(
    <TimingProvider>
        <MainApp/>
    </TimingProvider>
);
