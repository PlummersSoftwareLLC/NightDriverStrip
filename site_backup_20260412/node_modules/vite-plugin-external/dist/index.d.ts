import { Plugin } from 'vite';
export interface BasicOptions {
    externals?: Externals;
}
export interface Externals {
    [packageName: string]: any;
}
export interface Options extends BasicOptions {
    [mode: string]: Options | any;
    cwd?: string;
    cacheDir?: string;
    enforce?: 'pre' | 'post';
    externals?: Externals;
    development?: Options;
    production?: Options;
}
export default function createPlugin(opts: Options): Plugin;
