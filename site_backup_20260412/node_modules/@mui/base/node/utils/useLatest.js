"use strict";
'use client';

Object.defineProperty(exports, "__esModule", {
  value: true
});
exports.useLatest = useLatest;
var React = _interopRequireWildcard(require("react"));
function _getRequireWildcardCache(nodeInterop) { if (typeof WeakMap !== "function") return null; var cacheBabelInterop = new WeakMap(); var cacheNodeInterop = new WeakMap(); return (_getRequireWildcardCache = function (nodeInterop) { return nodeInterop ? cacheNodeInterop : cacheBabelInterop; })(nodeInterop); }
function _interopRequireWildcard(obj, nodeInterop) { if (!nodeInterop && obj && obj.__esModule) { return obj; } if (obj === null || typeof obj !== "object" && typeof obj !== "function") { return { default: obj }; } var cache = _getRequireWildcardCache(nodeInterop); if (cache && cache.has(obj)) { return cache.get(obj); } var newObj = {}; var hasPropertyDescriptor = Object.defineProperty && Object.getOwnPropertyDescriptor; for (var key in obj) { if (key !== "default" && Object.prototype.hasOwnProperty.call(obj, key)) { var desc = hasPropertyDescriptor ? Object.getOwnPropertyDescriptor(obj, key) : null; if (desc && (desc.get || desc.set)) { Object.defineProperty(newObj, key, desc); } else { newObj[key] = obj[key]; } } } newObj.default = obj; if (cache) { cache.set(obj, newObj); } return newObj; }
/**
 * @ignore - internal hook.
 *
 * Initializes a ref with the given value and updates it when the value changes.
 *
 * @param value Value to store in the ref
 * @param deps An optional array of dependencies to watch for changes. If not provided, the ref will be updated each time the `value` changes.
 * @returns A React.RefObject containing the latest value
 *
 * API:
 *
 * - [useLatest API](https://mui.com/base-ui/api/use-latest/)
 */
function useLatest(value, deps) {
  const ref = React.useRef(value);
  React.useEffect(() => {
    ref.current = value;
    // eslint-disable-next-line react-hooks/exhaustive-deps
  }, deps != null ? deps : [value]);
  return ref;
}