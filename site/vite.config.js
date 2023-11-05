import { defineConfig } from 'vite'
import react from '@vitejs/plugin-react'
import viteCompression from 'vite-plugin-compression';
import createExternal from 'vite-plugin-external';
import { viteStaticCopy } from 'vite-plugin-static-copy'

export default defineConfig({
    root: 'src',
    plugins: [
        react(), 
        createExternal({
            externals: {
                'react': 'React',
                'react-dom': 'ReactDOM',
                'recharts': "Recharts",
                '@mui/material': "MaterialUI",
                'html-react-parser': "HTMLReactParser",
            },
        }), 
        viteStaticCopy({
            targets: [
                {
                    src: '../../assets/favicon.ico',
                    dest: ''
                }
            ]
        }),
        viteCompression({
            deleteOriginFile: true,
            filter: '/\.(js|mjs|json|css|html|ico)$/i'
        })
    ],
    server: {
        port: 9000,
    },
    build: {
        outDir: '../dist',
        rollupOptions: {
            output: {
                entryFileNames: '[name].js',
                chunkFileNames: '[name].js',
                assetFileNames: '[name].[ext]',
                format: 'iife'
            },
        },
    }
})
