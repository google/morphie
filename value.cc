// Copyright 2015 Google Inc. All Rights Reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
// WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  See the
// License for the specific language governing permissions and limitations under
// the License.

// See third_party/logle/value_checker.h for details on values.
#include "third_party/logle/value.h"

#include <algorithm>

#include "third_party/logle/ast.h"
#include "third_party/logle/type_checker.h"
#include "third_party/logle/util/logging.h"
#include "third_party/logle/util/time_utils.h"
#include "third_party/logle/value_checker.h"

namespace third_party_logle {
namespace ast {
namespace value {

namespace {

const char kContainerSizeErr[] = "A size is only defined for containers.";
const char kListTypeErr[] = "The AST 'type' must be a list type.";
const char kSetTypeErr[] = "The AST 'type' must be a set type.";
const char kTupleTypeErr[] = "The AST 'type' must be a tuple type.";

void SetBoolIntervalBound(bool val, bool is_lower, AST* ast) {
  CHECK(ast != nullptr, "");
  AST* arg = ast->mutable_c_ast()->mutable_arg(is_lower ? 0 : 1);
  *arg = MakeBool(val);
}

void SetIntIntervalBound(int val, bool is_lower, AST* ast) {
  CHECK(ast != nullptr, "");
  AST* arg = ast->mutable_c_ast()->mutable_arg(is_lower ? 0 : 1);
  *arg = MakeInt(val);
}

void SetStringIntervalBound(const string& val, bool is_lower, AST* ast) {
  CHECK(ast != nullptr, "");
  AST* arg = ast->mutable_c_ast()->mutable_arg(is_lower ? 0 : 1);
  *arg = MakeString(val);
}

void SetTimestampIntervalBound(int val, bool is_lower, AST* ast) {
  CHECK(ast != nullptr, "");
  AST* arg = ast->mutable_c_ast()->mutable_arg(is_lower ? 0 : 1);
  *arg = MakeTimestampFromUnixMicros(val);
}

void CheckContainer(Operator op, const AST& type, const AST& arg,
                    const AST* const container) {
  if (op == Operator::LIST) {
    CHECK(ast::IsList(type), kListTypeErr);
  } else if (op == Operator::SET) {
    CHECK(ast::IsSet(type), kSetTypeErr);
  }
  string err;
  CHECK((type::IsType(type, &err)), err);
  CHECK(container != nullptr, "");
  CHECK((IsValue(*container, &err)), err);
  CHECK((type::IsTyped(type.c_ast().arg(0), arg, &err)), err);
}

void AppendToContainer(const AST& type, const AST& arg, AST* container) {
  AST* end = container->mutable_c_ast()->add_arg();
  *end = arg;
}

}  // namespace

AST MakeNull() {
  AST ast;
  return ast;
}

AST MakePrimitiveNull(PrimitiveType ptype) {
  AST ast = MakeNull();
  ast.mutable_p_ast()->set_type(ptype);
  return ast;
}

AST MakeBool(bool val) {
  AST ast = MakePrimitiveNull(PrimitiveType::BOOL);
  ast.mutable_p_ast()->mutable_val()->set_bool_val(val);
  return ast;
}

AST MakeInt(int val) {
  AST ast = MakePrimitiveNull(PrimitiveType::INT);
  ast.mutable_p_ast()->mutable_val()->set_int_val(val);
  return ast;
}

AST MakeString(const string& val) {
  AST ast = MakePrimitiveNull(PrimitiveType::STRING);
  ast.mutable_p_ast()->mutable_val()->set_string_val(val);
  return ast;
}

AST MakeTimestampFromUnixMicros(int64_t val) {
  AST ast = MakePrimitiveNull(PrimitiveType::TIMESTAMP);
  ast.mutable_p_ast()->mutable_val()->set_time_val(val);
  return ast;
}

AST MakeTimestampFromRFC3339(const string& val) {
  int64_t unix_micros;
  bool parsed = util::RFC3339ToUnixMicros(val, &unix_micros);
  if (parsed) {
    return MakeTimestampFromUnixMicros(unix_micros);
  }
  return MakePrimitiveNull(PrimitiveType::TIMESTAMP);
}

bool GetBool(const AST& val) {
  CHECK(val.has_p_ast(), "");
  CHECK(val.p_ast().has_val(), "");
  CHECK(val.p_ast().val().has_bool_val(), "");
  return val.p_ast().val().bool_val();
}

int GetInt(const AST& val) {
  CHECK(val.has_p_ast(), "");
  CHECK(val.p_ast().has_val(), "");
  CHECK(val.p_ast().val().has_int_val(), "");
  return val.p_ast().val().int_val();
}

string GetString(const AST& val) {
  CHECK(val.has_p_ast(), "");
  CHECK(val.p_ast().has_val(), "");
  CHECK(val.p_ast().val().has_string_val(), "");
  return val.p_ast().val().string_val();
}

int GetTimestamp(const AST& val) {
  CHECK(val.has_p_ast(), "");
  CHECK(val.p_ast().has_val(), "");
  CHECK(val.p_ast().val().has_time_val(), "");
  return val.p_ast().val().time_val();
}

AST MakeCompositeNull(Operator op) {
  AST ast;
  ast.mutable_c_ast()->set_op(op);
  return ast;
}

AST MakeEmptyInterval(PrimitiveType ptype) {
  AST ast = MakeCompositeNull(Operator::INTERVAL);
  return ast;
}

AST MakeMaxInterval(PrimitiveType ptype) {
  AST ast = MakeCompositeNull(Operator::INTERVAL);
  AST* arg = ast.mutable_c_ast()->add_arg();
  *arg = MakePrimitiveNull(ptype);
  arg = ast.mutable_c_ast()->add_arg();
  *arg = MakePrimitiveNull(ptype);
  return ast;
}

AST MakeBoolInterval(bool lower_val, bool upper_val) {
  AST ast = MakeMaxInterval(PrimitiveType::BOOL);
  SetBoolIntervalBound(lower_val, true, &ast);
  SetBoolIntervalBound(upper_val, false, &ast);
  return ast;
}

AST MakeBoolHalfInterval(bool val, bool is_lower) {
  AST ast = MakeMaxInterval(PrimitiveType::BOOL);
  SetBoolIntervalBound(val, is_lower, &ast);
  return ast;
}

AST MakeIntInterval(int lower_val, int upper_val) {
  AST ast = MakeMaxInterval(PrimitiveType::INT);
  SetIntIntervalBound(lower_val, true, &ast);
  SetIntIntervalBound(upper_val, false, &ast);
  return ast;
}

AST MakeIntHalfInterval(int val, bool is_lower) {
  AST ast = MakeMaxInterval(PrimitiveType::INT);
  SetIntIntervalBound(val, is_lower, &ast);
  return ast;
}

AST MakeStringInterval(const string& lower_val, const string& upper_val) {
  AST ast = MakeMaxInterval(PrimitiveType::STRING);
  SetStringIntervalBound(lower_val, true, &ast);
  SetStringIntervalBound(upper_val, false, &ast);
  return ast;
}

AST MakeStringHalfInterval(const string& val, bool is_lower) {
  AST ast = MakeMaxInterval(PrimitiveType::STRING);
  SetStringIntervalBound(val, is_lower, &ast);
  return ast;
}

AST MakeTimestampInterval(int lower_val, int upper_val) {
  AST ast = MakeMaxInterval(PrimitiveType::STRING);
  SetTimestampIntervalBound(lower_val, true, &ast);
  SetTimestampIntervalBound(upper_val, false, &ast);
  return ast;
}

AST MakeTimestampHalfInterval(int val, bool is_lower) {
  AST ast = MakeMaxInterval(PrimitiveType::TIMESTAMP);
  SetTimestampIntervalBound(val, is_lower, &ast);
  return ast;
}

int Size(const AST& container) {
  CHECK(ast::IsContainer(container), kContainerSizeErr);
  return container.c_ast().arg_size();
}

AST MakeEmptyList() { return MakeCompositeNull(Operator::LIST); }

void Append(const AST& type, const AST& arg, AST* list) {
  CheckContainer(Operator::LIST, type, arg, list);
  AppendToContainer(type, arg, list);
}

AST MakeEmptySet() { return MakeCompositeNull(Operator::SET); }

void Insert(const AST& type, const AST& arg, AST* set) {
  CheckContainer(Operator::SET, type, arg, set);
  AST new_arg = arg;
  Canonicalize(&new_arg);
  string arg_str = new_arg.SerializeAsString();

  bool has_arg = std::any_of(set->mutable_c_ast()->mutable_arg()->begin(),
                             set->mutable_c_ast()->mutable_arg()->end(),
                             [&arg_str](AST& old_arg) {
                               Canonicalize(&old_arg);
                               return old_arg.SerializeAsString() == arg_str;
                             });
  if (!has_arg) {
    AppendToContainer(type, new_arg, set);
  }
}

AST MakeNullTuple(int num_fields) {
  CHECK(num_fields >= 0, "");
  AST ast = MakeCompositeNull(Operator::TUPLE);
  for (int i = 0; i < num_fields; ++i) {
    ast.mutable_c_ast()->add_arg();
  }
  return ast;
}

void SetField(const AST& type, int field_num, const AST& arg, AST* tuple) {
  CHECK(ast::IsTuple(type), kTupleTypeErr);
  string err;
  CHECK((type::IsType(type, &err)), err);
  CHECK(tuple != nullptr, "");
  CHECK(field_num >= 0, "");
  CHECK(field_num < type.c_ast().arg_size(), "");
  CHECK(tuple->c_ast().arg_size() == type.c_ast().arg_size(), "");
  AST* field = tuple->mutable_c_ast()->mutable_arg(field_num);
  *field = arg;
}

}  // namespace value
}  // namespace ast
}  // namespace third_party_logle
