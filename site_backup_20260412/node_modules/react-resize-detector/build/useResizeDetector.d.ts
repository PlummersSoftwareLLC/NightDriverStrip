import { useRef, MutableRefObject } from 'react';
import { Props, ReactResizeDetectorDimensions } from './ResizeDetector';
interface FunctionProps extends Props {
    targetRef?: ReturnType<typeof useRef>;
}
declare function useResizeDetector<T = any>(props?: FunctionProps): UseResizeDetectorReturn<T>;
export default useResizeDetector;
export interface UseResizeDetectorReturn<T> extends ReactResizeDetectorDimensions {
    ref: MutableRefObject<T | null>;
}
