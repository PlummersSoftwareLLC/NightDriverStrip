import { defineConfig } from 'vite'
import preact from '@preact/preset-vite'
import viteCompression from 'vite-plugin-compression';
import { viteStaticCopy } from 'vite-plugin-static-copy'

export default defineConfig({
    root: 'src',
    plugins: [
        preact(),
        viteStaticCopy({
            targets: [{ src: '../../assets/favicon.ico', dest: '' }]
        }),
        viteCompression({
            filter:           /\.(js|mjs|json|css|html|ico)$/i,
            deleteOriginFile: true,
            threshold:        0,
        }),
    ],
    server: { port: 9000 },
    build: {
        outDir: '../dist',
        rollupOptions: {
            output: {
                entryFileNames: '[name].js',
                chunkFileNames: '[name].js',
                assetFileNames: '[name].[ext]',
                format: 'iife',
            },
        },
    },
})
