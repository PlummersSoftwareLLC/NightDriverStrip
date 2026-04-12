"use strict";
'use client';

Object.defineProperty(exports, "__esModule", {
  value: true
});
exports.useCompoundItem = useCompoundItem;
var React = _interopRequireWildcard(require("react"));
var _utils = require("@mui/utils");
var _useCompound = require("./useCompound");
function _getRequireWildcardCache(nodeInterop) { if (typeof WeakMap !== "function") return null; var cacheBabelInterop = new WeakMap(); var cacheNodeInterop = new WeakMap(); return (_getRequireWildcardCache = function (nodeInterop) { return nodeInterop ? cacheNodeInterop : cacheBabelInterop; })(nodeInterop); }
function _interopRequireWildcard(obj, nodeInterop) { if (!nodeInterop && obj && obj.__esModule) { return obj; } if (obj === null || typeof obj !== "object" && typeof obj !== "function") { return { default: obj }; } var cache = _getRequireWildcardCache(nodeInterop); if (cache && cache.has(obj)) { return cache.get(obj); } var newObj = {}; var hasPropertyDescriptor = Object.defineProperty && Object.getOwnPropertyDescriptor; for (var key in obj) { if (key !== "default" && Object.prototype.hasOwnProperty.call(obj, key)) { var desc = hasPropertyDescriptor ? Object.getOwnPropertyDescriptor(obj, key) : null; if (desc && (desc.get || desc.set)) { Object.defineProperty(newObj, key, desc); } else { newObj[key] = obj[key]; } } } newObj.default = obj; if (cache) { cache.set(obj, newObj); } return newObj; }
/**
 * Registers a child component with the parent component.
 *
 * @param id A unique key for the child component. If the `id` is `undefined`, the registration logic will not run (this can sometimes be the case during SSR).
 * @param itemMetadata Arbitrary metadata to pass to the parent component. This should be a stable reference (e.g. a memoized object), to avoid unnecessary re-registrations.
 * @param missingKeyGenerator A function that generates a unique id for the item.
 *   It is called with the set of the ids of all the items that have already been registered.
 *   Return `existingKeys.size` if you want to use the index of the new item as the id.
 *
 * @ignore - internal hook.
 */

function useCompoundItem(id, itemMetadata) {
  const context = React.useContext(_useCompound.CompoundComponentContext);
  if (context === null) {
    throw new Error('useCompoundItem must be used within a useCompoundParent');
  }
  const {
    registerItem
  } = context;
  const [registeredId, setRegisteredId] = React.useState(typeof id === 'function' ? undefined : id);
  (0, _utils.unstable_useEnhancedEffect)(() => {
    const {
      id: returnedId,
      deregister
    } = registerItem(id, itemMetadata);
    setRegisteredId(returnedId);
    return deregister;
  }, [registerItem, itemMetadata, id]);
  return {
    id: registeredId,
    index: registeredId !== undefined ? context.getItemIndex(registeredId) : -1,
    totalItemCount: context.totalSubitemCount
  };
}