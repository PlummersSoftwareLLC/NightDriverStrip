# vite-plugin-external

[![npm package](https://nodei.co/npm/vite-plugin-external.png?downloads=true&downloadRank=true&stars=true)](https://www.npmjs.com/package/vite-plugin-external)

> The `vite-plugin-external` provides a way of excluding dependencies from the runtime code and output bundles.

[![NPM version](https://img.shields.io/npm/v/vite-plugin-external.svg?style=flat)](https://npmjs.org/package/vite-plugin-external)
[![NPM Downloads](https://img.shields.io/npm/dm/vite-plugin-external.svg?style=flat)](https://npmjs.org/package/vite-plugin-external)

![image](https://user-images.githubusercontent.com/6262382/126889725-a5d276ad-913a-4498-8da1-2aa3fd1404ab.png)

## Installation

```bash
npm install vite-plugin-external --save-dev
```

## Options

* `externals: [packageName: string]: any`
* `development?: Options`
* `production?: Options`
* `cwd?: string` - default: `process.cwd()`
* `cacheDir?: string` - default: `join(cwd, 'node_modules', '.vite_external')`

## Usage

```html
<script src="
//cdn.jsdelivr.net/npm/react@16.14.0/umd/react.production.min.js"></script>
```

```js
import createExternal from 'vite-plugin-external';

export default defineConfig({
  plugins: [
    createExternal({
      externals: {
        react: 'React'
      }
    })
  ],
  build: {
    cssCodeSplit: false,
    rollupOptions: {
      output: {
        manualChunks: undefined,
        assetFileNames: 'assets/[name][extname]',
        entryFileNames: '[name].js',
        format: 'iife'
      }
    }
  }
});
```

### Override externals by mode

```html
<script src="
//g.alicdn.com/linkdesign/lib/1.0.1/~react.js"></script>
```

```js
import createExternal from 'vite-plugin-external';

export default defineConfig({
  plugins: [
    createExternal({
      externals: {
        react: 'React'
      },
      development: {
        externals: {
          react: '$linkdesign.React'
        }
      }
    })
  ]
});
```

## Examples

**[Demo](examples/react)**
