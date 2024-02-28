#ifndef NODE_TREE_SITTER_QUERY_H_
#define NODE_TREE_SITTER_QUERY_H_

#include "./addon_data.h"

#include <napi.h>
#include <node_object_wrap.h>
#include <tree_sitter/api.h>

namespace node_tree_sitter {

class Query : public Napi::ObjectWrap<Query> {
 public:
  static constexpr napi_type_tag TYPE_TAG = {
    0x0B236F23456A4B87, 0x86BAC1E58CA6897B
  };

  static void Init(Napi::Env env, Napi::Object exports);
  static Query *UnwrapQuery(const Napi::Value &);

  explicit Query(const Napi::CallbackInfo &info);
  ~Query() override;

 private:
  TSQuery *query_;

  Napi::Value New(const Napi::CallbackInfo &);
  Napi::Value Matches(const Napi::CallbackInfo &);
  Napi::Value Captures(const Napi::CallbackInfo &);
  Napi::Value GetPredicates(const Napi::CallbackInfo &);
};

}  // namespace node_tree_sitter

#endif  // NODE_TREE_SITTER_QUERY_H_
