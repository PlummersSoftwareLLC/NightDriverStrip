import * as React from 'react';
import { ComponentType } from 'react';
import { ComponentsProps } from './ResizeDetector';
declare function withResizeDetector<P, ElementT extends HTMLElement = HTMLElement>(ComponentInner: ComponentType<P>, options?: ComponentsProps<ElementT>): React.ForwardRefExoticComponent<React.PropsWithoutRef<Without<Without<OptionalKey<P, "targetRef">, "width">, "height">> & React.RefAttributes<HTMLElement>>;
declare type Without<T, Key> = Key extends keyof T ? Omit<T, Key> : T;
declare type OptionalKey<T, Key> = Key extends keyof T ? Omit<T, Key> & {
    [K in Key]?: T[K];
} : T;
export default withResizeDetector;
