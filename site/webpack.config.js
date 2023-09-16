const path = require("path");
const HtmlWebpackPlugin = require("html-webpack-plugin");
const CopyWebpackPlugin = require('copy-webpack-plugin');

module.exports = {
  entry: './src/main.jsx', // main entry point for loading the site
  output: {
    path: path.join(__dirname, "/dist"), // the bundle output path
    filename: "index.js", // the name of the bundle
    // libraryTarget: 'window'
  },
  // externals: {
  //   'react': 'React',
  //   "react-dom": 'ReactDOM',
  //   "@mui/material": "MaterialUI",
  //   "recharts": "Recharts",
  // },
  devServer: {
    static: {
      directory: path.join(__dirname, 'dist'),
    },
    compress: true,
    port: 9000,
  },
  plugins: [
    new HtmlWebpackPlugin({
      template: "src/index.html", // to import index.html file inside index.js
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
  optimization: { // TODO Move to development
    minimize: false
  },
  resolve: {
    extensions: [".*", ".js", ".jsx"]
  }
};