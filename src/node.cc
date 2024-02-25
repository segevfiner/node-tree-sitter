#include "./node.h"
#include <napi.h>
#include <tree_sitter/api.h>
#include <vector>
#include "./util.h"
#include "./conversions.h"
#include "./tree.h"
#include "./tree_cursor.h"

using std::vector;
using namespace Napi;

namespace node_tree_sitter {
namespace node_methods {

static const uint32_t FIELD_COUNT_PER_NODE = 6;

static inline void setup_transfer_buffer(Napi::Env env, uint32_t node_count) {
  auto data = env.GetInstanceData<AddonData>();

  uint32_t new_length = node_count * FIELD_COUNT_PER_NODE;
  if (new_length > data->transfer_buffer_length) {
    data->transfer_buffer_length = new_length;

    auto js_transfer_buffer = Uint32Array::New(env, data->transfer_buffer_length);
    data->transfer_buffer = js_transfer_buffer.Data();

    data->module_exports.Value()["nodeTransferArray"] = js_transfer_buffer;
  }
}

static inline bool operator<=(const TSPoint &left, const TSPoint &right) {
  if (left.row < right.row) return true;
  if (left.row > right.row) return false;
  return left.column <= right.column;
}

static Napi::Value MarshalNodes(const Napi::CallbackInfo &info,
                         const Tree *tree, const TSNode *nodes, uint32_t node_count) {
  return GetMarshalNodes(info, tree, nodes, node_count);
}

Napi::Value MarshalNode(const Napi::CallbackInfo &info, const Tree *tree, TSNode node) {
  return GetMarshalNode(info, tree, node);
}

Napi::Value GetMarshalNodes(const Napi::CallbackInfo &info,
                         const Tree *tree, const TSNode *nodes, uint32_t node_count) {
  Env env = info.Env();
  auto data = env.GetInstanceData<AddonData>();
  auto result = Array::New(env, node_count);
  setup_transfer_buffer(env, node_count);
  uint32_t *p = data->transfer_buffer;
  for (unsigned i = 0; i < node_count; i++) {
    TSNode node = nodes[i];
    const auto &cache_entry = tree->cached_nodes_.find(node.id);
    if (cache_entry == tree->cached_nodes_.end()) {
      MarshalNodeId(node.id, p);
      p += 2;
      *(p++) = node.context[0];
      *(p++) = node.context[1];
      *(p++) = node.context[2];
      *(p++) = node.context[3];
      if (node.id) {
        result[i] = Number::New(env, ts_node_symbol(node));
      } else {
        result[i] = env.Null();
      }
    } else {
      result[i] = cache_entry->second->node.Value();
    }
  }
  return result;
}

Napi::Value GetMarshalNode(const Napi::CallbackInfo &info, const Tree *tree, TSNode node) {
  Env env = info.Env();
  AddonData* data = env.GetInstanceData<AddonData>();
  const auto &cache_entry = tree->cached_nodes_.find(node.id);
  if (cache_entry == tree->cached_nodes_.end()) {
    setup_transfer_buffer(env, 1);
    uint32_t *p = data->transfer_buffer;
    MarshalNodeId(node.id, p);
    p += 2;
    *(p++) = node.context[0];
    *(p++) = node.context[1];
    *(p++) = node.context[2];
    *(p++) = node.context[3];
    if (node.id) {
      return Number::New(env, ts_node_symbol(node));
    }
  } else {
    return cache_entry->second->node.Value();
  }
  return env.Null();
}

static Napi::Value MarshalNullNode(Napi::Env env) {
  auto data = env.GetInstanceData<AddonData>();
  memset(data->transfer_buffer, 0, FIELD_COUNT_PER_NODE * sizeof(data->transfer_buffer[0]));
  return env.Undefined();
}

TSNode UnmarshalNode(Napi::Env env, const Tree *tree) {
  AddonData* data = env.GetInstanceData<AddonData>();
  TSNode result = {{0, 0, 0, 0}, nullptr, nullptr};
  result.tree = tree->tree_;
  if (!result.tree) {
    throw TypeError::New(env, "Argument must be a tree");
  }

  result.id = UnmarshalNodeId(&data->transfer_buffer[0]);
  result.context[0] = data->transfer_buffer[2];
  result.context[1] = data->transfer_buffer[3];
  result.context[2] = data->transfer_buffer[4];
  result.context[3] = data->transfer_buffer[5];
  return result;
}

static Napi::Value ToString(const Napi::CallbackInfo &info) {
  Env env = info.Env();
  const Tree *tree = Tree::UnwrapTree(info[0]);
  TSNode node = UnmarshalNode(env, tree);
  if (node.id) {
    char *string = ts_node_string(node);
    String result = String::New(env, string);
    free(string);
    return result;
  }

  return env.Undefined();
}

static Napi::Value IsMissing(const Napi::CallbackInfo &info) {
  Env env = info.Env();
  const Tree *tree = Tree::UnwrapTree(info[0]);
  TSNode node = UnmarshalNode(env, tree);
  if (node.id) {
    bool result = ts_node_is_missing(node);
    return Boolean::New(env, result);
  }

  return env.Undefined();
}

static Napi::Value HasChanges(const Napi::CallbackInfo &info) {
  Env env = info.Env();
  const Tree *tree = Tree::UnwrapTree(info[0]);
  TSNode node = UnmarshalNode(env, tree);
  if (node.id) {
    bool result = ts_node_has_changes(node);
    return Boolean::New(env, result);
  }

  return env.Undefined();
}

static Napi::Value HasError(const Napi::CallbackInfo &info) {
  Env env = info.Env();
  const Tree *tree = Tree::UnwrapTree(info[0]);
  TSNode node = UnmarshalNode(env, tree);
  if (node.id) {
    bool result = ts_node_has_error(node);
    return Boolean::New(env, result);
  }

  return env.Undefined();
}

static Napi::Value FirstNamedChildForIndex(const Napi::CallbackInfo &info) {
  Env env = info.Env();
  const Tree *tree = Tree::UnwrapTree(info[0]);
  TSNode node = UnmarshalNode(env, tree);
  if (node.id) {
    Napi::Maybe<uint32_t> byte = ByteCountFromJS(info[1]);
    if (byte.IsJust()) {
      return MarshalNode(info, tree, ts_node_first_named_child_for_byte(node, byte.Unwrap()));
    }
  }
  return MarshalNullNode(env);
}

static Napi::Value FirstChildForIndex(const Napi::CallbackInfo &info) {
  Env env = info.Env();
  const Tree *tree = Tree::UnwrapTree(info[0]);
  TSNode node = UnmarshalNode(env, tree);

  if (node.id && info.Length() > 1) {
    Napi::Maybe<uint32_t> byte = ByteCountFromJS(info[1]);
    if (byte.IsJust()) {
      return MarshalNode(info, tree, ts_node_first_child_for_byte(node, byte.Unwrap()));
    }
  }
  return MarshalNullNode(env);
}

static Napi::Value NamedDescendantForIndex(const Napi::CallbackInfo &info) {
  Env env = info.Env();
  const Tree *tree = Tree::UnwrapTree(info[0]);
  TSNode node = UnmarshalNode(env, tree);

  if (node.id) {
    Napi::Maybe<uint32_t> maybe_min = ByteCountFromJS(info[1]);
    Napi::Maybe<uint32_t> maybe_max = ByteCountFromJS(info[2]);
    if (maybe_min.IsJust() && maybe_max.IsJust()) {
      uint32_t min = maybe_min.Unwrap();
      uint32_t max = maybe_max.Unwrap();
      return MarshalNode(info, tree, ts_node_named_descendant_for_byte_range(node, min, max));
    }
  }
  return MarshalNullNode(env);
}

static Napi::Value DescendantForIndex(const Napi::CallbackInfo &info) {
  Env env = info.Env();
  const Tree *tree = Tree::UnwrapTree(info[0]);
  TSNode node = UnmarshalNode(env, tree);

  if (node.id) {
    Napi::Maybe<uint32_t> maybe_min = ByteCountFromJS(info[1]);
    Napi::Maybe<uint32_t> maybe_max = ByteCountFromJS(info[2]);
    if (maybe_min.IsJust() && maybe_max.IsJust()) {
      uint32_t min = maybe_min.Unwrap();
      uint32_t max = maybe_max.Unwrap();
      return MarshalNode(info, tree, ts_node_descendant_for_byte_range(node, min, max));
    }
  }
  return MarshalNullNode(env);
}

static Napi::Value NamedDescendantForPosition(const Napi::CallbackInfo &info) {
  Env env = info.Env();
  const Tree *tree = Tree::UnwrapTree(info[0]);
  TSNode node = UnmarshalNode(env, tree);

  if (node.id) {
    Napi::Maybe<TSPoint> maybe_min = PointFromJS(info[1]);
    Napi::Maybe<TSPoint> maybe_max = PointFromJS(info[2]);
    if (maybe_min.IsJust() && maybe_max.IsJust()) {
      TSPoint min = maybe_min.Unwrap();
      TSPoint max = maybe_max.Unwrap();
      return MarshalNode(info, tree, ts_node_named_descendant_for_point_range(node, min, max));
    }
  }
  return MarshalNullNode(env);
}

static Napi::Value DescendantForPosition(const Napi::CallbackInfo &info) {
  Env env = info.Env();
  const Tree *tree = Tree::UnwrapTree(info[0]);
  TSNode node = UnmarshalNode(env, tree);

  if (node.id) {
    Napi::Maybe<TSPoint> maybe_min = PointFromJS(info[1]);
    Napi::Maybe<TSPoint> maybe_max = PointFromJS(info[2]);
    if (maybe_min.IsJust() && maybe_max.IsJust()) {
      TSPoint min = maybe_min.Unwrap();
      TSPoint max = maybe_max.Unwrap();
      return MarshalNode(info, tree, ts_node_descendant_for_point_range(node, min, max));
    }
  }
  return MarshalNullNode(env);
}

static Napi::Value Type(const Napi::CallbackInfo &info) {
  Env env = info.Env();
  const Tree *tree = Tree::UnwrapTree(info[0]);
  TSNode node = UnmarshalNode(env, tree);

  if (node.id) {
    return String::New(env, ts_node_type(node));
  }

  return env.Undefined();
}

static Napi::Value TypeId(const Napi::CallbackInfo &info) {
  Env env = info.Env();
  const Tree *tree = Tree::UnwrapTree(info[0]);
  TSNode node = UnmarshalNode(env, tree);

  if (node.id) {
    return Number::New(env, ts_node_symbol(node));
  }

  return env.Undefined();
}

static Napi::Value IsNamed(const Napi::CallbackInfo &info) {
  Env env = info.Env();
  const Tree *tree = Tree::UnwrapTree(info[0]);
  TSNode node = UnmarshalNode(env, tree);

  if (node.id) {
    return Boolean::New(env, ts_node_is_named(node));
  }

  return env.Undefined();
}

static Napi::Value StartIndex(const Napi::CallbackInfo &info) {
  Env env = info.Env();
  const Tree *tree = Tree::UnwrapTree(info[0]);
  TSNode node = UnmarshalNode(env, tree);

  if (node.id) {
    int32_t result = ts_node_start_byte(node) / 2;
    return Number::New(env, result);
  }

  return env.Undefined();
}

static Napi::Value EndIndex(const Napi::CallbackInfo &info) {
  Env env = info.Env();
  const Tree *tree = Tree::UnwrapTree(info[0]);
  TSNode node = UnmarshalNode(env, tree);

  if (node.id) {
    int32_t result = ts_node_end_byte(node) / 2;
    return Number::New(env, result);
  }

  return env.Undefined();
}

static Napi::Value StartPosition(const Napi::CallbackInfo &info) {
  Env env = info.Env();
  const Tree *tree = Tree::UnwrapTree(info[0]);
  TSNode node = UnmarshalNode(env, tree);

  if (node.id) {
    TransferPoint(env, ts_node_start_point(node));
  }

  return env.Undefined();
}

static Napi::Value EndPosition(const Napi::CallbackInfo &info) {
  Env env = info.Env();
  const Tree *tree = Tree::UnwrapTree(info[0]);
  TSNode node = UnmarshalNode(env, tree);

  if (node.id) {
    TransferPoint(env, ts_node_end_point(node));
  }

  return env.Undefined();
}

static Napi::Value Child(const Napi::CallbackInfo &info) {
  Env env = info.Env();
  const Tree *tree = Tree::UnwrapTree(info[0]);
  TSNode node = UnmarshalNode(env, tree);

  if (node.id) {
    if (!info[1].IsNumber()) {
      throw TypeError::New(env, "Second argument must be an integer");
    }
    uint32_t index = info[1].As<Number>().Uint32Value();
    return MarshalNode(info, tree, ts_node_child(node, index));
  }
  return MarshalNullNode(env);
}

static Napi::Value NamedChild(const Napi::CallbackInfo &info) {
  Env env = info.Env();
  const Tree *tree = Tree::UnwrapTree(info[0]);
  TSNode node = UnmarshalNode(env, tree);

  if (node.id) {
    if (!info[1].IsNumber()) {
      throw TypeError::New(env, "Second argument must be an integer");
    }
    uint32_t index = info[1].As<Number>().Uint32Value();
    return MarshalNode(info, tree, ts_node_named_child(node, index));
  }
  return MarshalNullNode(env);
}

static Napi::Value ChildCount(const Napi::CallbackInfo &info) {
  Env env = info.Env();
  const Tree *tree = Tree::UnwrapTree(info[0]);
  TSNode node = UnmarshalNode(env, tree);

  if (node.id) {
    return Number::New(env, ts_node_child_count(node));
  }

  return env.Undefined();
}

static Napi::Value NamedChildCount(const Napi::CallbackInfo &info) {
  Env env = info.Env();
  const Tree *tree = Tree::UnwrapTree(info[0]);
  TSNode node = UnmarshalNode(env, tree);

  if (node.id) {
    return Number::New(env, ts_node_named_child_count(node));
  }

  return env.Undefined();
}

static Napi::Value FirstChild(const Napi::CallbackInfo &info) {
  Env env = info.Env();
  const Tree *tree = Tree::UnwrapTree(info[0]);
  TSNode node = UnmarshalNode(env, tree);
  if (node.id) {
    return MarshalNode(info, tree, ts_node_child(node, 0));
  }
  return MarshalNullNode(env);
}

static Napi::Value FirstNamedChild(const Napi::CallbackInfo &info) {
  Env env = info.Env();
  const Tree *tree = Tree::UnwrapTree(info[0]);
  TSNode node = UnmarshalNode(env, tree);
  if (node.id) {
    return MarshalNode(info, tree, ts_node_named_child(node, 0));
  }
  return MarshalNullNode(env);
}

static Napi::Value LastChild(const Napi::CallbackInfo &info) {
  Env env = info.Env();
  const Tree *tree = Tree::UnwrapTree(info[0]);
  TSNode node = UnmarshalNode(env, tree);
  if (node.id) {
    uint32_t child_count = ts_node_child_count(node);
    if (child_count > 0) {
      return MarshalNode(info, tree, ts_node_child(node, child_count - 1));
    }
  }
  return MarshalNullNode(env);
}

static Napi::Value LastNamedChild(const Napi::CallbackInfo &info) {
  Env env = info.Env();
  const Tree *tree = Tree::UnwrapTree(info[0]);
  TSNode node = UnmarshalNode(env, tree);
  if (node.id) {
    uint32_t child_count = ts_node_named_child_count(node);
    if (child_count > 0) {
      return MarshalNode(info, tree, ts_node_named_child(node, child_count - 1));
    }
  }
  return MarshalNullNode(env);
}

static Napi::Value Parent(const Napi::CallbackInfo &info) {
  Env env = info.Env();
  const Tree *tree = Tree::UnwrapTree(info[0]);
  TSNode node = UnmarshalNode(env, tree);
  if (node.id) {
    return MarshalNode(info, tree, ts_node_parent(node));
  }
  return MarshalNullNode(env);
}

static Napi::Value NextSibling(const Napi::CallbackInfo &info) {
  Env env = info.Env();
  const Tree *tree = Tree::UnwrapTree(info[0]);
  TSNode node = UnmarshalNode(env, tree);
  if (node.id) {
    return MarshalNode(info, tree, ts_node_next_sibling(node));
  }
  return MarshalNullNode(env);
}

static Napi::Value NextNamedSibling(const Napi::CallbackInfo &info) {
  Env env = info.Env();
  const Tree *tree = Tree::UnwrapTree(info[0]);
  TSNode node = UnmarshalNode(env, tree);
  if (node.id) {
    return MarshalNode(info, tree, ts_node_next_named_sibling(node));
  }
  return MarshalNullNode(env);
}

static Napi::Value PreviousSibling(const Napi::CallbackInfo &info) {
  Env env = info.Env();
  const Tree *tree = Tree::UnwrapTree(info[0]);
  TSNode node = UnmarshalNode(env, tree);
  if (node.id) {
    return MarshalNode(info, tree, ts_node_prev_sibling(node));
  }
  return MarshalNullNode(env);
}

static Napi::Value PreviousNamedSibling(const Napi::CallbackInfo &info) {
  Env env = info.Env();
  const Tree *tree = Tree::UnwrapTree(info[0]);
  TSNode node = UnmarshalNode(env, tree);
  if (node.id) {
    return MarshalNode(info, tree, ts_node_prev_named_sibling(node));
  }
  return MarshalNullNode(env);
}

struct SymbolSet {
  std::basic_string<TSSymbol> symbols;
  void add(TSSymbol symbol) { symbols += symbol; }
  bool contains(TSSymbol symbol) { return symbols.find(symbol) != symbols.npos; }
};

void symbol_set_from_js(SymbolSet *symbols, const Napi::Value &value, const TSLanguage *language) {
  Env env = value.Env();

  if (!value.IsArray()) {
    throw TypeError::New(env, "Argument must be a string or array of strings");
  }

  unsigned symbol_count = ts_language_symbol_count(language);

  auto js_types = value.As<Array>();
  for (unsigned i = 0, n = js_types.Length(); i < n; i++) {
    Value js_node_type_value = js_types[i];
    if (js_node_type_value.IsString()) {
      String js_node_type = js_node_type_value.As<String>();
      std::string node_type = js_node_type.Utf8Value();

      if (node_type == "ERROR") {
        symbols->add(static_cast<TSSymbol>(-1));
      } else {
        for (TSSymbol j = 0; j < symbol_count; j++) {
          if (node_type == ts_language_symbol_name(language, j)) {
            symbols->add(j);
          }
        }
      }

      continue;
    }

    throw TypeError::New(env, "Argument must be a string or array of strings");
  }
}

static Napi::Value Children(const Napi::CallbackInfo &info) {
  Env env = info.Env();
  AddonData* data = env.GetInstanceData<AddonData>();
  const Tree *tree = Tree::UnwrapTree(info[0]);
  TSNode node = UnmarshalNode(env, tree);
  if (!node.id) return;

  vector<TSNode> result;
  ts_tree_cursor_reset(&data->scratch_cursor, node);
  if (ts_tree_cursor_goto_first_child(&data->scratch_cursor)) {
    do {
      TSNode child = ts_tree_cursor_current_node(&data->scratch_cursor);
      result.push_back(child);
    } while (ts_tree_cursor_goto_next_sibling(&data->scratch_cursor));
  }

  return MarshalNodes(info, tree, result.data(), result.size());
}

static Napi::Value NamedChildren(const Napi::CallbackInfo &info) {
  Env env = info.Env();
  AddonData* data = env.GetInstanceData<AddonData>();
  const Tree *tree = Tree::UnwrapTree(info[0]);
  TSNode node = UnmarshalNode(env, tree);
  if (!node.id) return;

  vector<TSNode> result;
  ts_tree_cursor_reset(&data->scratch_cursor, node);
  if (ts_tree_cursor_goto_first_child(&data->scratch_cursor)) {
    do {
      TSNode child = ts_tree_cursor_current_node(&data->scratch_cursor);
      if (ts_node_is_named(child)) {
        result.push_back(child);
      }
    } while (ts_tree_cursor_goto_next_sibling(&data->scratch_cursor));
  }

  return MarshalNodes(info, tree, result.data(), result.size());
}

static Napi::Value DescendantsOfType(const Napi::CallbackInfo &info) {
  Env env = info.Env();
  AddonData* data = env.GetInstanceData<AddonData>();
  const Tree *tree = Tree::UnwrapTree(info[0]);
  TSNode node = UnmarshalNode(env, tree);
  if (!node.id) return;

  SymbolSet symbols;
  symbol_set_from_js(&symbols, info[1], ts_tree_language(node.tree));

  TSPoint start_point = {0, 0};
  TSPoint end_point = {UINT32_MAX, UINT32_MAX};

  if (info.Length() > 2 && info[2].IsObject()) {
    auto maybe_start_point = PointFromJS(info[2]);
    if (maybe_start_point.IsNothing()) return;
    start_point = maybe_start_point.Unwrap();
  }

  if (info.Length() > 3 && info[3].IsObject()) {
    auto maybe_end_point = PointFromJS(info[3]);
    if (maybe_end_point.IsNothing()) return;
    end_point = maybe_end_point.Unwrap();
  }

  vector<TSNode> found;
  ts_tree_cursor_reset(&data->scratch_cursor, node);
  auto already_visited_children = false;
  while (true) {
    TSNode descendant = ts_tree_cursor_current_node(&data->scratch_cursor);

    if (!already_visited_children) {
      if (ts_node_end_point(descendant) <= start_point) {
        if (ts_tree_cursor_goto_next_sibling(&data->scratch_cursor)) {
          already_visited_children = false;
        } else {
          if (!ts_tree_cursor_goto_parent(&data->scratch_cursor)) break;
          already_visited_children = true;
        }
        continue;
      }

      if (end_point <= ts_node_start_point(descendant)) break;

      if (symbols.contains(ts_node_symbol(descendant))) {
        found.push_back(descendant);
      }

      if (ts_tree_cursor_goto_first_child(&data->scratch_cursor)) {
        already_visited_children = false;
      } else if (ts_tree_cursor_goto_next_sibling(&data->scratch_cursor)) {
        already_visited_children = false;
      } else {
        if (!ts_tree_cursor_goto_parent(&data->scratch_cursor)) break;
        already_visited_children = true;
      }
    } else {
      if (ts_tree_cursor_goto_next_sibling(&data->scratch_cursor)) {
        already_visited_children = false;
      } else {
        if (!ts_tree_cursor_goto_parent(&data->scratch_cursor)) break;
      }
    }
  }

  return MarshalNodes(info, tree, found.data(), found.size());
}

static Napi::Value ChildNodesForFieldId(const Napi::CallbackInfo &info) {
  Env env = info.Env();
  AddonData* data = env.GetInstanceData<AddonData>();
  const Tree *tree = Tree::UnwrapTree(info[0]);
  TSNode node = UnmarshalNode(env, tree);
  if (!node.id) return;

  if (!info[1].IsNumber()) {
    throw TypeError::New(env, "Second argument must be an integer");
  }
  auto maybe_field_id = info[1].As<Number>();
  uint32_t field_id = maybe_field_id.Uint32Value();

  vector<TSNode> result;
  ts_tree_cursor_reset(&data->scratch_cursor, node);
  if (ts_tree_cursor_goto_first_child(&data->scratch_cursor)) {
    do {
      TSNode child = ts_tree_cursor_current_node(&data->scratch_cursor);
      if (ts_tree_cursor_current_field_id(&data->scratch_cursor) == field_id) {
        result.push_back(child);
      }
    } while (ts_tree_cursor_goto_next_sibling(&data->scratch_cursor));
  }

  return MarshalNodes(info, tree, result.data(), result.size());
}

static Napi::Value ChildNodeForFieldId(const Napi::CallbackInfo &info) {
  Env env = info.Env();
  const Tree *tree = Tree::UnwrapTree(info[0]);
  TSNode node = UnmarshalNode(env, tree);

  if (node.id) {
    if (!info[1].IsNumber()) {
      throw TypeError::New(env, "Second argument must be an integer");
    }
    auto maybe_field_id = info[1].As<Number>();
    uint32_t field_id = maybe_field_id.Uint32Value();
    return MarshalNode(info, tree, ts_node_child_by_field_id(node, field_id));
  }
  return MarshalNullNode(env);
}

static Napi::Value Closest(const Napi::CallbackInfo &info) {
  Env env = info.Env();
  const Tree *tree = Tree::UnwrapTree(info[0]);
  TSNode node = UnmarshalNode(env, tree);
  if (!node.id) return;

  SymbolSet symbols;
  symbol_set_from_js(&symbols, info[1], ts_tree_language(node.tree));

  for (;;) {
    TSNode parent = ts_node_parent(node);
    if (!parent.id) break;
    if (symbols.contains(ts_node_symbol(parent))) {
      return MarshalNode(info, tree, parent);
    }
    node = parent;
  }

  return MarshalNullNode(env);
}

static Napi::Value Walk(const Napi::CallbackInfo &info) {
  Env env = info.Env();
  const Tree *tree = Tree::UnwrapTree(info[0]);
  TSNode node = UnmarshalNode(env, tree);
  TSTreeCursor cursor = ts_tree_cursor_new(node);
  return TreeCursor::NewInstance(env, cursor);
}

void Init(Napi::Env env, Napi::Object exports) {
  auto data = env.GetInstanceData<AddonData>();

  Object result = Object::New(env);

  FunctionPair methods[] = {
    {"startIndex", StartIndex},
    {"endIndex", EndIndex},
    {"type", Type},
    {"typeId", TypeId},
    {"isNamed", IsNamed},
    {"parent", Parent},
    {"child", Child},
    {"namedChild", NamedChild},
    {"children", Children},
    {"namedChildren", NamedChildren},
    {"childCount", ChildCount},
    {"namedChildCount", NamedChildCount},
    {"firstChild", FirstChild},
    {"lastChild", LastChild},
    {"firstNamedChild", FirstNamedChild},
    {"lastNamedChild", LastNamedChild},
    {"nextSibling", NextSibling},
    {"nextNamedSibling", NextNamedSibling},
    {"previousSibling", PreviousSibling},
    {"previousNamedSibling", PreviousNamedSibling},
    {"startPosition", StartPosition},
    {"endPosition", EndPosition},
    {"isMissing", IsMissing},
    {"toString", ToString},
    {"firstChildForIndex", FirstChildForIndex},
    {"firstNamedChildForIndex", FirstNamedChildForIndex},
    {"descendantForIndex", DescendantForIndex},
    {"namedDescendantForIndex", NamedDescendantForIndex},
    {"descendantForPosition", DescendantForPosition},
    {"namedDescendantForPosition", NamedDescendantForPosition},
    {"hasChanges", HasChanges},
    {"hasError", HasError},
    {"descendantsOfType", DescendantsOfType},
    {"walk", Walk},
    {"closest", Closest},
    {"childNodeForFieldId", ChildNodeForFieldId},
    {"childNodesForFieldId", ChildNodesForFieldId},
  };

  for (size_t i = 0; i < length_of_array(methods); i++) {
    result[methods[i].name] = Napi::Function::New(env, methods[i].callback);
  }

  data->module_exports = Napi::Persistent(exports);
  setup_transfer_buffer(env, 1);

  exports["NodeMethods"] = result;
}

}  // namespace node_methods
}  // namespace node_tree_sitter
