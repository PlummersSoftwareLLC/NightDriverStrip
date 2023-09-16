const path = require("path");
const HtmlWebpackPlugin = require("html-webpack-plugin");
const CopyWebpackPlugin = require('copy-webpack-plugin');

module.exports = (env, argv) => {
    const optimization = argv.mode === 'development' ? {minimize: false} : {} 
    let externals;
    let index;
    // For offline mode, or development offline run wepback with --env offline. 
    // dependencies will be compiled into the bundle, it will be larger, but no
    // requirement for internet needed.  
    if (env.offline) {
        externals = {}
        index = 'index.html'
    } else {
        index = 'index.html'
        externals = {
            'react': 'React',
            "react-dom": 'ReactDOM',
            "recharts": "Recharts",
            // TODO Work out how to use @mui/styles with a CDN. It causes the UI to fail.
        };
    }
    return {
        entry: './src/main.jsx', // main entry point for loading the site
        output: {
            path: path.join(__dirname, "/dist"), // the bundle output path
            filename: "index.js", // the name of the bundle
        },
        externals,
        devServer: {
            static: {
                directory: path.join(__dirname, 'dist'),
            },
            compress: true,
            port: 9000,
        },
        plugins: [
            new HtmlWebpackPlugin({
                template: `src/${index}`, // to import index.html file inside index.js
            }),
            new CopyWebpackPlugin({
                patterns: [
                    { from: "../assets/favicon.ico" },
                ],
            }),
        ],
        module: {
            rules: [
                {
                    test: /\.(js|jsx)/,
                    exclude: /node_modules/,
                    use: ["babel-loader"],
                },
            ],
        },
        optimization,
        resolve: {
            extensions: [".*", ".js", ".jsx"]
        }
    }
};