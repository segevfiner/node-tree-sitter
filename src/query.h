#ifndef NODE_TREE_SITTER_QUERY_H_
#define NODE_TREE_SITTER_QUERY_H_

#include <napi.h>
#include <node_object_wrap.h>
#include <unordered_map>
#include <tree_sitter/api.h>
#include "./addon_data.h"

namespace node_tree_sitter {

class Query : public Napi::ObjectWrap<Query> {
 public:
  static void Init(Napi::Env env, Napi::Object exports);
  static Query *UnwrapQuery(const Napi::Value &);

  explicit Query(const CallbackInfo &info);
  ~Query();

  TSQuery *query_;

 private:

  Napi::Value New(const Napi::CallbackInfo &);
  Napi::Value Matches(const Napi::CallbackInfo &);
  Napi::Value Captures(const Napi::CallbackInfo &);
  Napi::Value GetPredicates(const Napi::CallbackInfo &);
};

}  // namespace node_tree_sitter

#endif  // NODE_TREE_SITTER_QUERY_H_
