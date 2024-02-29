#ifndef NODE_TREE_SITTER_TREE_H_
#define NODE_TREE_SITTER_TREE_H_

#include "./addon_data.h"

#include <napi.h>
#include <node_object_wrap.h>
#include <tree_sitter/api.h>
#include <unordered_map>

namespace node_tree_sitter {

class Tree : public Napi::ObjectWrap<Tree> {
 public:
  // tstag() {
  //   b2sum -l64 <(printf tree-sitter) <(printf "$1") | \
  //   awk '{printf "0x" toupper($1) (NR == 1 ? ", " : "\n")}'
  // }
  // tstag tree # => 0x8AF2E5212AD58ABF, 0x7FA28BFC1966AC2D
  static constexpr napi_type_tag TYPE_TAG = {
    0x8AF2E5212AD58ABF, 0x7FA28BFC1966AC2D
  };

  static void Init(Napi::Env env, Napi::Object exports);
  static Napi::Value NewInstance(Napi::Env env, TSTree *);
  static const Tree *UnwrapTree(const Napi::Value &);

  explicit Tree(const Napi::CallbackInfo &);
  ~Tree() override;

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
