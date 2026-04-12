import { ReactElement, ReactNode, Component, SVGProps } from 'react';
import { CartesianViewBox, TickItem, PresentationAttributesAdaptChildEvent } from '../util/types';
interface CartesianTickItem extends TickItem {
    tickCoord?: number;
    tickSize?: number;
    isShow?: boolean;
}
export interface CartesianAxisProps {
    className?: string;
    x?: number;
    y?: number;
    width?: number;
    height?: number;
    unit?: string | number;
    orientation?: 'top' | 'bottom' | 'left' | 'right';
    viewBox?: CartesianViewBox;
    tick?: SVGProps<SVGTextElement> | ReactElement<SVGElement> | ((props: any) => ReactElement<SVGElement>) | boolean;
    axisLine?: boolean | SVGProps<SVGLineElement>;
    tickLine?: boolean | SVGProps<SVGLineElement>;
    mirror?: boolean;
    tickMargin?: number;
    hide?: boolean;
    label?: any;
    minTickGap?: number;
    ticks?: CartesianTickItem[];
    tickSize?: number;
    tickFormatter?: (value: any, index: number) => string;
    ticksGenerator?: (props?: CartesianAxisProps) => CartesianTickItem[];
    interval?: number | 'preserveStart' | 'preserveEnd' | 'preserveStartEnd';
}
interface IState {
    fontSize: string;
    letterSpacing: string;
}
export declare type Props = Omit<PresentationAttributesAdaptChildEvent<any, SVGElement>, 'viewBox'> & CartesianAxisProps;
export declare class CartesianAxis extends Component<Props, IState> {
    static displayName: string;
    static defaultProps: {
        x: number;
        y: number;
        width: number;
        height: number;
        viewBox: {
            x: number;
            y: number;
            width: number;
            height: number;
        };
        orientation: string;
        ticks: CartesianTickItem[];
        stroke: string;
        tickLine: boolean;
        axisLine: boolean;
        tick: boolean;
        mirror: boolean;
        minTickGap: number;
        tickSize: number;
        tickMargin: number;
        interval: string;
    };
    private layerReference;
    constructor(props: Props);
    static getTicks(props: Props, fontSize?: string, letterSpacing?: string): any[];
    static getNumberIntervalTicks(ticks: CartesianTickItem[], interval: number): CartesianTickItem[];
    static getTicksStart({ ticks, tickFormatter, viewBox, orientation, minTickGap, unit, fontSize, letterSpacing, }: Omit<Props, 'tickMargin'>, preserveEnd?: boolean): CartesianTickItem[];
    static getTicksEnd({ ticks, tickFormatter, viewBox, orientation, minTickGap, unit, fontSize, letterSpacing, }: Omit<Props, 'tickMargin'>): CartesianTickItem[];
    shouldComponentUpdate({ viewBox, ...restProps }: Props, nextState: IState): boolean;
    componentDidMount(): void;
    getTickLineCoord(data: CartesianTickItem): {
        line: {
            x1: number;
            y1: number;
            x2: number;
            y2: number;
        };
        tick: {
            x: number;
            y: number;
        };
    };
    getTickTextAnchor(): string;
    getTickVerticalAnchor(): string;
    renderAxisLine(): JSX.Element;
    static renderTickItem(option: Props['tick'], props: any, value: ReactNode): JSX.Element;
    renderTicks(ticks: CartesianTickItem[], fontSize: string, letterSpacing: string): JSX.Element;
    render(): JSX.Element;
}
export {};
