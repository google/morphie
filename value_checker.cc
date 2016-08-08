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

// The methods for checking if an AST is a value walk an AST. Methods for
// checking if two ASTs are isomorphic walk two ASTs simultaneously. Both have
// running time linear in the size of the argument ASTs. Only interval and set
// values can have redundant representations, so the canonicalization methods
// walk an AST to find an interval or set and then apply a transformation.
#include "value_checker.h"

#include <algorithm>
#include <vector>

#include "google/protobuf/message_lite.h"
#include "ast.h"
#include "util/logging.h"
#include "util/string_utils.h"

namespace tervuren {
namespace ast {
namespace value {

namespace {

bool IsValueInternal(const AST& ast, const string& path, string* err);
bool IsInterval(const CompositeAST& c_ast, const string& path, string* err);

// This method is called IsContainerLike instead of IsContainer because it only
// checks if all the arguments of a composite node are values. It does not check
// if they have the same type.
bool IsContainerLike(const CompositeAST& c_ast, const string& path,
                     string* err);

bool IsValueInternal(const AST& ast, const string& path, string* err) {
  if (ast::IsNull(ast)) {
    return true;
  }
  if (ast.has_p_ast()) {
    if (!ast.p_ast().has_val()) {
      return true;
    }
    bool is_value = IsPrimitive(ast.p_ast().type(), ast.p_ast().val());
    if (!is_value) {
      string type_str = ast::ToString(ast.p_ast().type());
      string val_str = ast::ToString(ast.p_ast().val());
      *err = util::StrCat("The value of ", path, ".", type_str,
                          " is of the wrong type.");
    }
    return is_value;
  }
  if (ast.c_ast().arg_size() == 0) {
    return true;
  }
  string new_path = util::StrCat(path, ".", ast::ToString(ast.c_ast().op()));
  switch (ast.c_ast().op()) {
    case Operator::INTERVAL:
      return IsInterval(ast.c_ast(), new_path, err);
    case Operator::LIST:
      return IsContainerLike(ast.c_ast(), new_path, err);
    case Operator::TUPLE:
      return IsContainerLike(ast.c_ast(), new_path, err);
    case Operator::SET:
      return IsContainerLike(ast.c_ast(), new_path, err);
  }
}

bool IsInterval(const CompositeAST& c_ast, const string& path, string* err) {
  CHECK(c_ast.op() == Operator::INTERVAL, "");
  CHECK(c_ast.arg_size() != 0, "");
  if (c_ast.arg_size() == 2) {
    if (c_ast.arg(0).has_p_ast() && c_ast.arg(1).has_p_ast()) {
      AST lower_bound = c_ast.arg(0);
      AST upper_bound = c_ast.arg(1);
      if (lower_bound.p_ast().type() == upper_bound.p_ast().type()) {
        string new_path = util::StrCat(path, ".", "lower-bound");
        if (!IsValueInternal(lower_bound, new_path, err)) {
          return false;
        }
        new_path = util::StrCat(path, ".", "upper-bound");
        return IsValueInternal(upper_bound, new_path, err);
      }
    }
    *err = util::StrCat("The arguments to ", path,
                        " must have the same, ordered type.");
  } else {
    *err = util::StrCat("The interval ", path,
                        " must have zero or two arguments.");
  }
  return false;
}

bool IsContainerLike(const CompositeAST& c_ast, const string& path,
                     string* err) {
  CHECK(c_ast.arg_size() != 0, "");
  bool is_value = true;
  string new_path;
  for (int i = 0; i < c_ast.arg_size() && is_value; ++i) {
    new_path = util::StrCat(path, "(", std::to_string(i), ")");
    is_value = (is_value && IsValueInternal(c_ast.arg(i), new_path, err));
  }
  return is_value;
}

bool IsomorphicPrimitive(PrimitiveAST val1, PrimitiveAST val2) {
  if (val1.type() == val2.type()) {
    if (!val1.has_val() && !val2.has_val()) {
      return true;
    }
    if (val1.has_val() && val2.has_val()) {
      switch (val1.type()) {
        case PrimitiveType::BOOL:
          return val1.val().bool_val() == val2.val().bool_val();
        case PrimitiveType::INT:
          return val1.val().int_val() == val2.val().int_val();
        case PrimitiveType::STRING:
          return val1.val().string_val() == val2.val().string_val();
        case PrimitiveType::TIMESTAMP:
          return val1.val().time_val() == val2.val().time_val();
      }
    }
  }
  return false;
}

bool IsomorphicComposite(CompositeAST val1, CompositeAST val2) {
  if (val1.op() != val2.op() || val1.arg_size() != val2.arg_size()) {
    return false;
  }
  bool is_isomorphic = true;
  for (int i = 0; i < val1.arg_size() && is_isomorphic; ++i) {
    is_isomorphic = Isomorphic(val1.arg(i), val2.arg(i));
  }
  return is_isomorphic;
}

void CanonicalizeInterval(CompositeAST* val) {
  CHECK(val->op() == Operator::INTERVAL, "");
  CHECK(val->arg_size() == 2, "");
  CHECK(val->arg(0).has_p_ast(), "");
  CHECK(val->arg(1).has_p_ast(), "");
  if (LT(val->arg(1).p_ast(), val->arg(0).p_ast())) {
    val->clear_arg();
  }
}

void CanonicalizeContainer(CompositeAST* val) {
  CHECK(val->op() == Operator::LIST || val->op() == Operator::TUPLE, "");
  for (AST& ast : *(val->mutable_arg())) {
    Canonicalize(&ast);
  }
}

// Sort the argument list of a set and remove duplicates.
void CanonicalizeSet(CompositeAST* val) {
  CHECK(val->op() == Operator::SET, "");
  vector<string> elements;
  elements.reserve(val->arg_size());
  string serialized_val;
  for (AST& arg : *(val->mutable_arg())) {
    Canonicalize(&arg);
    arg.SerializeToString(&serialized_val);
    elements.push_back(serialized_val);
  }
  std::sort(elements.begin(), elements.end());
  auto end_itr = std::unique(elements.begin(), elements.end());
  elements.erase(end_itr, elements.end());
  val->clear_arg();
  AST arg;
  AST* arg_ptr;
  for (const auto& str_val : elements) {
    arg.ParseFromString(str_val);
    arg_ptr = val->add_arg();
    *arg_ptr = arg;
  }
}

void CanonicalizeComposite(CompositeAST* val) {
  if (val->arg_size() == 0) {
    return;
  }
  switch (val->op()) {
    case Operator::INTERVAL:
      return CanonicalizeInterval(val);
    case Operator::LIST:
      return CanonicalizeContainer(val);
    case Operator::SET:
      return CanonicalizeSet(val);
    case Operator::TUPLE:
      return CanonicalizeContainer(val);
  }
}

}  // namespace

bool IsValue(const AST& ast, string* err) {
  CHECK(err != nullptr, "");
  err->clear();
  return IsValueInternal(ast, "", err);
}

bool IsPrimitive(const PrimitiveType ptype, const PrimitiveValue& val) {
  switch (ptype) {
    case PrimitiveType::BOOL:
      return val.has_bool_val();
    case PrimitiveType::INT:
      return val.has_int_val();
    case PrimitiveType::STRING:
      return val.has_string_val();
    case PrimitiveType::TIMESTAMP:
      return val.has_time_val();
  }
}

bool LT(const PrimitiveAST& val1, const PrimitiveAST& val2) {
  CHECK(val1.type() == val2.type(), "");
  CHECK(val1.has_val() && val2.has_val(), "");
  CHECK(IsPrimitive(val1.type(), val1.val()), "");
  CHECK(IsPrimitive(val2.type(), val2.val()), "");
  switch (val1.type()) {
    case PrimitiveType::BOOL:
      return (!val1.val().bool_val() && val2.val().bool_val());
    case PrimitiveType::INT:
      return (val1.val().int_val() < val2.val().int_val());
    case PrimitiveType::STRING:
      return (val1.val().string_val() < val2.val().string_val());
    case PrimitiveType::TIMESTAMP:
      return (val1.val().time_val() < val2.val().time_val());
  }
}

bool Isomorphic(const AST& val1, const AST& val2) {
  string tmp_err;
  CHECK(IsValue(val1, &tmp_err), "");
  CHECK(IsValue(val1, &tmp_err), "");
  if (ast::IsNull(val1) && ast::IsNull(val2)) {
    return true;
  } else if (val1.has_p_ast() && val2.has_p_ast()) {
    return IsomorphicPrimitive(val1.p_ast(), val2.p_ast());
  } else if (val1.has_c_ast() && val2.has_c_ast()) {
    return IsomorphicComposite(val1.c_ast(), val2.c_ast());
  } else {
    return false;
  }
}

void Canonicalize(AST* val) {
  CHECK(val != nullptr, "");
  string tmp_err;
  CHECK(IsValue(*val, &tmp_err), "");
  if (val->has_c_ast()) {
    CanonicalizeComposite(val->mutable_c_ast());
  }
}

}  // namespace value
}  // namespace ast
}  // namespace tervuren
