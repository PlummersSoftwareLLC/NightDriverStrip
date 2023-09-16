import MainApp from './components/home/home'
import { StrictMode } from 'react';
import ReactDOM from 'react-dom';
import { createRoot } from 'react-dom/client';
createRoot(document.getElementById("root"))
  .render(<StrictMode><MainApp/></StrictMode>);