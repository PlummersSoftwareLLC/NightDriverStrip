import { PureComponent, ReactNode, RefObject } from 'react';
export interface ReactResizeDetectorDimensions {
    height?: number;
    width?: number;
}
interface ChildFunctionProps<ElementT extends HTMLElement> extends ReactResizeDetectorDimensions {
    targetRef?: RefObject<ElementT>;
}
export interface Props {
    /**
     * Function that will be invoked with observable element's width and height.
     * Default: undefined
     */
    onResize?: (width?: number, height?: number) => void;
    /**
     * Trigger update on height change.
     * Default: true
     */
    handleHeight?: boolean;
    /**
     * Trigger onResize on width change.
     * Default: true
     */
    handleWidth?: boolean;
    /**
     * Do not trigger update when a component mounts.
     * Default: false
     */
    skipOnMount?: boolean;
    /**
     * Changes the update strategy. Possible values: "throttle" and "debounce".
     * See `lodash` docs for more information https://lodash.com/docs/
     * undefined - callback will be fired for every frame.
     * Default: undefined
     */
    refreshMode?: 'throttle' | 'debounce';
    /**
     * Set the timeout/interval for `refreshMode` strategy
     * Default: undefined
     */
    refreshRate?: number;
    /**
     * Pass additional params to `refreshMode` according to lodash docs
     * Default: undefined
     */
    refreshOptions?: {
        leading?: boolean;
        trailing?: boolean;
    };
    /**
     * These options will be used as a second parameter of `resizeObserver.observe` method
     * @see https://developer.mozilla.org/en-US/docs/Web/API/ResizeObserver/observe
     * Default: undefined
     */
    observerOptions?: ResizeObserverOptions;
}
export interface ComponentsProps<ElementT extends HTMLElement> extends Props {
    /**
     * A selector of an element to observe.
     * You can use this property to attach resize-observer to any DOM element.
     * Please refer to the querySelector docs.
     * Default: undefined
     * @deprecated since version 5.0.0. It will be removed in version 7.0.0.
     * Use targetRef instead
     */
    querySelector?: string;
    /**
     * Valid only for a callback-pattern.
     * Ignored for other render types.
     * Set resize-detector's node type.
     * You can pass any valid React node: string with node's name or element.
     * Can be useful when you need to handle table's or paragraph's resizes.
     * Default: "div"
     * @deprecated since version 5.0.0. It will be removed in version 7.0.0.
     * Use targetRef instead
     */
    nodeType?: keyof JSX.IntrinsicElements;
    /**
     * A DOM element to observe.
     * By default it's a parent element in relation to the ReactResizeDetector component.
     * But you can pass any DOM element to observe.
     * This property is omitted when you pass querySelector.
     * Default: undefined
     * @deprecated since version 5.0.0. It will be removed in version 6.0.0.
     * Use targetRef instead
     */
    targetDomEl?: ElementT;
    /**
     * A React reference of the element to observe.
     * Pass a reference to the element you want to attach resize handlers to.
     * It must be an instance of React.useRef or React.createRef functions
     * Default: undefined
     */
    targetRef?: RefObject<ElementT>;
    render?: (props: ReactResizeDetectorDimensions) => ReactNode;
    children?: ReactNode | ((props: ChildFunctionProps<ElementT>) => ReactNode);
}
declare class ResizeDetector<ElementT extends HTMLElement = HTMLElement> extends PureComponent<ComponentsProps<ElementT>, ReactResizeDetectorDimensions> {
    skipOnMount: boolean | undefined;
    targetRef: any;
    observableElement: any;
    resizeHandler: any;
    resizeObserver: any;
    constructor(props: ComponentsProps<ElementT>);
    componentDidMount(): void;
    componentDidUpdate(): void;
    componentWillUnmount(): void;
    cancelHandler: () => void;
    attachObserver: () => void;
    getElement: () => Element | Text | null;
    createResizeHandler: ResizeObserverCallback;
    getRenderType: () => string;
    render(): any;
}
export default ResizeDetector;
