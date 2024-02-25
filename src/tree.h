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

  explicit Tree(const Napi::CallbackInfo &);
  ~Tree();

  struct NodeCacheEntry {
    Tree *tree;
    const void *key;
    Napi::ObjectReference node;
  };

  TSTree *tree_;
  std::unordered_map<const void *, NodeCacheEntry *> cached_nodes_;

 private:
  Napi::Value Edit(const Napi::CallbackInfo &);
  Napi::Value RootNode(const Napi::CallbackInfo &);
  Napi::Value PrintDotGraph(const Napi::CallbackInfo &);
  Napi::Value GetEditedRange(const Napi::CallbackInfo &);
  Napi::Value GetChangedRanges(const Napi::CallbackInfo &);
  Napi::Value CacheNode(const Napi::CallbackInfo &);
  Napi::Value CacheNodes(const Napi::CallbackInfo &);

};

}  // namespace node_tree_sitter

#endif  // NODE_TREE_SITTER_TREE_H_
