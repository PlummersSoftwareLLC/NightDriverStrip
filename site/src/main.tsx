import { createRoot } from "react-dom/client";
import { MainApp } from "./components/home/MainApp";
import { StrictMode } from "react";

const rootNode = document.getElementById("root");
if (rootNode == null) {
        throw new Error("Could not find root element");
}

createRoot(rootNode).render(<StrictMode><MainApp /></StrictMode>);