import MainApp from './components/home/home'
import { StrictMode } from 'react';
import { createRoot } from 'react-dom/client';

createRoot(document.getElementById("root"))
    .render(<StrictMode><MainApp/></StrictMode>);