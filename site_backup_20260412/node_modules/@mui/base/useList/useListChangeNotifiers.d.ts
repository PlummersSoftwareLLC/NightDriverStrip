export interface ListChangeNotifiers<ItemValue> {
    /**
     * Calls all the registered selection change handlers.
     *
     * @param newValue - The newly selected value(s).
     */
    notifySelectionChanged: (newValue: ItemValue[]) => void;
    /**
     * Calls all the registered highlight change handlers.
     *
     * @param newValue - The newly highlighted value.
     */
    notifyHighlightChanged: (newValue: ItemValue | null) => void;
    registerSelectionChangeHandler: (handler: (newValue: ItemValue[]) => void) => () => void;
    registerHighlightChangeHandler: (handler: (newValue: ItemValue | null) => void) => () => void;
}
/**
 * @ignore - internal hook.
 *
 * This hook is used to notify any interested components about changes in the list's selection and highlight.
 */
export declare function useListChangeNotifiers<Item>(): ListChangeNotifiers<Item>;
