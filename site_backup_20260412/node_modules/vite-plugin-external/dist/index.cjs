'use strict';

var node_fs = require('node:fs');
var fsExtra = require('fs-extra');
var node_path = require('node:path');

// compat cjs and esm
function createCJSExportDeclaration(external) {
    return `module.exports = ${external};`;
}
function rollupOutputGlobals(output, externals) {
    let { globals } = output;
    if (!globals) {
        globals = {};
        output.globals = globals;
    }
    Object.assign(globals, externals);
}
function rollupExternal(rollupOptions, externals, externalKeys) {
    let { output, external } = rollupOptions;
    if (!output) {
        output = {};
        rollupOptions.output = output;
    }
    // compat Array
    if (Array.isArray(output)) {
        output.forEach((n) => {
            rollupOutputGlobals(n, externals);
        });
    }
    else {
        rollupOutputGlobals(output, externals);
    }
    // if external indicates
    if (!external) {
        external = [];
        rollupOptions.external = external;
    }
    external.push(...externalKeys);
}
function createPlugin(opts) {
    const cwd = opts.cwd || process.cwd();
    const externalCacheDir = opts.cacheDir || node_path.join(cwd, 'node_modules', '.vite_external');
    let externals = {};
    let externalKeys = [];
    let shouldSkip = false;
    return {
        name: 'vite-plugin-external',
        enforce: opts.enforce,
        config(config, { mode }) {
            const modeOptions = opts[mode];
            externals = Object.assign({}, opts.externals, modeOptions && modeOptions.externals);
            externalKeys = Object.keys(externals);
            shouldSkip = !externalKeys.length;
            if (shouldSkip) {
                return;
            }
            // non development
            if (mode !== 'development') {
                let { build } = config;
                // if no build indicates
                if (!build) {
                    build = {};
                    config.build = build;
                }
                let { rollupOptions } = build;
                // if no rollupOptions indicates
                if (!rollupOptions) {
                    rollupOptions = {};
                    build.rollupOptions = rollupOptions;
                }
                rollupExternal(rollupOptions, externals, externalKeys);
                return;
            }
            if (!node_fs.existsSync) {
                node_fs.mkdirSync(externalCacheDir);
            }
            else {
                fsExtra.emptyDirSync(externalCacheDir);
            }
            let { resolve } = config;
            if (!resolve) {
                resolve = {};
                config.resolve = resolve;
            }
            let { alias } = resolve;
            if (!alias || typeof alias !== 'object') {
                alias = [];
                resolve.alias = alias;
            }
            // #1 if alias is object type
            if (!Array.isArray(alias)) {
                alias = Object.entries(alias).map(([key, value]) => {
                    return { find: key, replacement: value };
                });
                resolve.alias = alias;
            }
            for (const libName of externalKeys) {
                const libPath = node_path.join(externalCacheDir, `${libName.replace(/\//g, '_')}.js`);
                node_fs.writeFileSync(libPath, createCJSExportDeclaration(externals[libName]));
                alias.push({
                    find: new RegExp(`^${libName}$`),
                    replacement: libPath
                });
            }
        }
    };
}

module.exports = createPlugin;
