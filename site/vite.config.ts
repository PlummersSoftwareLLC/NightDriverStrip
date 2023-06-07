// default vite config
import { defineConfig } from 'vite'
import react from '@vitejs/plugin-react'
import { viteExternalsPlugin } from 'vite-plugin-externals';

const viteExternals = viteExternalsPlugin({
    react: 'React',
    'react-dom': 'ReactDOM',
    'react-router-dom': 'ReactRouterDOM',
    'react-router': 'ReactRouter',
    'material-ui': 'MaterialUI',
    '@material-ui': 'MaterialUI',
    "recharts": "Recharts",
    "@mui": "MaterialUI"
})

export default defineConfig({
    plugins: [react(), viteExternals],
    server: {
        port: 3000,
    },
    publicDir: './local',
    build: {
        outDir: './dist',
        sourcemap: true,
        rollupOptions: {
            output: {
                entryFileNames: 'assets/[name].js',
                chunkFileNames: 'assets/[name].js',
                assetFileNames: 'assets/[name].[ext]'
            }
        },
    },
});