#ifndef NODE_TREE_SITTER_QUERY_H_
#define NODE_TREE_SITTER_QUERY_H_

#include "./addon_data.h"

#include <napi.h>
#include <node_object_wrap.h>
#include <tree_sitter/api.h>

namespace node_tree_sitter {

class Query : public Napi::ObjectWrap<Query> {
 public:
  // tstag() {
  //   b2sum -l64 <(printf tree-sitter) <(printf "$1") | \
  //   awk '{printf "0x" toupper($1) (NR == 1 ? ", " : "\n")}'
  // }
  // tstag query # => 0x8AF2E5212AD58ABF, 0x7B1FAB666CBD6803
  static constexpr napi_type_tag TYPE_TAG = {
    0x8AF2E5212AD58ABF, 0x7B1FAB666CBD6803
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
