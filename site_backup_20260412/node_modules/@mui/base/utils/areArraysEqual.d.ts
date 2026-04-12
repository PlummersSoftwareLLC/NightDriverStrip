type ItemComparer<Item> = (a: Item, b: Item) => boolean;
export declare function areArraysEqual<Item>(array1: Item[], array2: Item[], itemComparer?: ItemComparer<Item>): boolean;
export {};
