// TODO(https://github.com/microsoft/vscode-languageserver-node/pull/1644):
// remove this file when a release containing this PR is available.

export {};

declare module 'vscode-jsonrpc/lib/common/linkedMap' {
  interface LinkedMap<K, V> {
    entries(): MapIterator<[K, V]>;
    keys(): MapIterator<K>;
    values(): MapIterator<V>;
    [Symbol.iterator](): MapIterator<[K, V]>;
  }
}
