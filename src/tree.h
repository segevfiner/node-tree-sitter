#ifndef NODE_TREE_SITTER_TREE_H_
#define NODE_TREE_SITTER_TREE_H_

#include <napi.h>
#include <node_object_wrap.h>
#include <unordered_map>
#include <tree_sitter/api.h>
#include "./addon_data.h"

namespace node_tree_sitter {

class Tree : public Napi::ObjectWrap<Tree> {
 public:
  static void Init(Napi::Env env, Napi::Object exports);
  static Napi::Value NewInstance(Napi::Env env, TSTree *);
  static const Tree *UnwrapTree(const Napi::Value &);

  struct NodeCacheEntry {
    Tree *tree;
    const void *key;
    Napi::ObjectReference node;
  };

  TSTree *tree_;
  std::unordered_map<const void *, NodeCacheEntry *> cached_nodes_;

 private:
  explicit Tree(TSTree *);
  ~Tree();

  static void New(const Nan::FunctionCallbackInfo<v8::Value> &);
  static void Edit(const Nan::FunctionCallbackInfo<v8::Value> &);
  static void RootNode(const Nan::FunctionCallbackInfo<v8::Value> &);
  static void PrintDotGraph(const Nan::FunctionCallbackInfo<v8::Value> &);
  static void GetEditedRange(const Nan::FunctionCallbackInfo<v8::Value> &);
  static void GetChangedRanges(const Nan::FunctionCallbackInfo<v8::Value> &);
  static void CacheNode(const Nan::FunctionCallbackInfo<v8::Value> &);
  static void CacheNodes(const Nan::FunctionCallbackInfo<v8::Value> &);

};

}  // namespace node_tree_sitter

#endif  // NODE_TREE_SITTER_TREE_H_
