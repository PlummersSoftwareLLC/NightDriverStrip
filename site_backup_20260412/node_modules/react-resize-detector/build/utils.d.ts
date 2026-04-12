/// <reference types="lodash" />
import debounce from 'lodash/debounce';
import throttle from 'lodash/throttle';
import { Props, ReactResizeDetectorDimensions } from './ResizeDetector';
export declare type patchResizeHandlerType = ReturnType<typeof debounce> | ReturnType<typeof throttle> | ResizeObserverCallback;
export declare const patchResizeHandler: (resizeCallback: ResizeObserverCallback, refreshMode: Props['refreshMode'], refreshRate: Props['refreshRate'], refreshOptions: Props['refreshOptions']) => patchResizeHandlerType;
export declare const isFunction: (fn: any) => boolean;
export declare const isSSR: () => boolean;
export declare const isDOMElement: (element: any) => boolean;
export declare const createNotifier: (onResize: Props['onResize'], setSize: React.Dispatch<React.SetStateAction<ReactResizeDetectorDimensions>>, handleWidth: boolean, handleHeight: boolean) => ({ width, height }: ReactResizeDetectorDimensions) => void;
