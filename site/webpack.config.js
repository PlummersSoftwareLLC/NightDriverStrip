const path = require("path");
const HtmlWebpackPlugin = require("html-webpack-plugin");
const CopyWebpackPlugin = require('copy-webpack-plugin');
const CompressionPlugin = require("compression-webpack-plugin");

module.exports = (env, argv) => {
    let optimization = {} 
    let devtool = undefined
    let externals;
    let index;
    // For offline mode, or development offline run wepback with --env offline. 
    // dependencies will be compiled into the bundle, it will be larger, but no
    // requirement for internet needed.  
    if (env.offline) {
        externals = {}
        index = 'index.html.offline'
    } else {
        index = 'index.html'
        externals = {
            'react': 'React',
            "react-dom": 'ReactDOM',
            "recharts": "Recharts",
            "@mui/material": "MaterialUI",
            'html-react-parser': "HTMLReactParser",
        };
    }
    const plugins = [
        new HtmlWebpackPlugin({
            template: `src/${index}`,
            filename: 'index.html'
        }),
        new CopyWebpackPlugin({
            patterns: [
                { from: "../assets/favicon.ico" },
            ],
        }),
    ];
    if (argv.mode === 'development') {
        optimization = {minimize: false}
        devtool = 'source-map'
    } else {
        plugins.push(new CompressionPlugin({
            minRatio: 1,
            deleteOriginalAssets: false
        }))
    }
    return {
        entry: './src/main.jsx', // main entry point for loading the site
        output: {
            path: path.join(__dirname, "/dist"), // the bundle output path
            filename: "index.js", // the name of the bundle
        },
        devServer: {
            static: {
                directory: path.join(__dirname, 'dist'),
            },
            compress: true,
            port: 9000,
        },
        externals,
        devtool,
        plugins,
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