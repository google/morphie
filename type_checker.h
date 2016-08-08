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

// A type is an Abstract Syntax Tree (AST) representing a type in the grammar
// given in ast.proto. A value is an AST representing an instance of a type.
// The ASTs for types and values are similar but not identical.
//
// Example. This code constructs two ASTs. The AST 'interval' represents an
// integer interval type. The AST 'val' first represents the empty interval, is
// modified to an AST representing '[0,', which is not an interval because it
// lacks an upper bound, and finally represents the interval '[0, 5]'.
// This example demonstrates how functions defined in this file are used to
// check if an AST represents a type and if a value is of some type.
//
//   // Create a type.
//   AST interval;
//   interval.set_is_nullable(true);
//   interval.set_name("duration");
//   interval.mutable_c_ast()->set_op(Operator::INTERVAL);
//   AST* arg = interval.mutable_c_ast()->add_arg();
//   arg->is_nullable(false);
//   arg->name("bounds");
//   arg->mutable_p_ast()->set_type(PrimitiveType::INT);
//   string err;
//   CHECK(IsType(interval, &err));
//
//   // Create a value.
//   AST val;
//   val.mutable_c_ast()->set_op(Operator::INTERVAL);
//   CHECK(IsTyped(interval, val, &err)); // The empty interval is typed.
//   Value* arg = interval.mutable_c_ast()->add_arg();
//   arg->mutable_p_ast()->set_type(PrimitiveType::INT);
//   arg->mutable_p_ast()->set_int_val(0);
//   CHECK(!IsTyped(interval, val, &err)); // Type check fails.
//   arg = interval.mutable_c_ast()->add_arg();
//   arg->mutable_p_ast()->set_type(PrimitiveType::INT);
//   arg->mutable_p_ast()->set_int_val(2);
//   CHECK(IsTyped(interval, val, &err));
//
// The 'name' and 'is_nullable' fields of an AST must be set for a type but cann
// be omitted in a value. In an AST representing a type, an operator field
// represents a type constructor. In an AST representing a value, the operator
// field represents an operator on values. In both type and value ASTs, the
// number of arguments should match what is required by the operator. See the
// documentation of IsType below for when an AST represents a type.
//
// Example. Two ASTs are shown in pretty printing notation below.
//   Type : interval(int)   Value : interval(int(0), int(4))
// In the type, 'interval' is a type constructor and must have one argument. In
// the value, 'interval' is an operator and must have either zero arguments for
// the empty interval, or two arguments for a lower and an upper bound.
//
// There are subtle differences between nullable types. The three types below
// represent a nullable list of ints, a list of nullable ints and a nullable
// list of nullable ints. Two lists are alsow show: the empty list and a list of
// four elements, two of which are empty.
// 1. list?(int)  2. list(int?) 3. list?(int?)
// a. ()          b. (,1,,3)
//
// List a. will satisfy the first and third type but not the second. List b.
// will satisfy the second and third type but not the first.
#ifndef LOGLE_TYPE_CHECKER_H_
#define LOGLE_TYPE_CHECKER_H_

#include <map>

#include "ast.pb.h"
#include "base/string.h"

namespace tervuren {
namespace ast {
namespace type {

using Types = std::map<string, AST>;

// The functions in this file allow for checking if an AST represents a type and
// for checking if a value is of a given type. The running time of these
// functions is linear in the size of the argument ASTs.
// An AST represents a type if it has the fields 'name' and
// 'is_nullable' and satisfies these conditions:
//   - An AST for the null type has no primitive or composite AST.
//   - A primitive AST is a type.
//   - An interval must have exactly one primitive type as an argument.
//   - A list or a set must have exactly one argument that is a type.
//   - A tuple must have one or more arguments, each of which is a type.
// The function IsType:
//   - Returns true if 'ast' represents a type.
//   - Returns false otherwise, with an error explanation in '*err'.
//   - Requires 'err' to be non-null.
bool IsType(const AST& ast, string* err);

  // Returns 'true' if all ASTs in Types represent types.
  // - Requires 'err' to be non-null.
  // The running time is linear in the sum of the ASTs in 'types'.
bool AreTypes(const Types& types, string* err);

  // Returns a string to which the contents of 'types' has been pretty printed.
string ToString(const Types& types);

  // Returns 'true' if the 'val' is of type 'type' and 'false' otherwise with
  // a reason in '*err'.
  //   - Requires 'err' to be non-null.
bool IsTyped(const AST& type, const AST& val, string* err);
  // Returns 'true' if the tag of 'val' exists in the map 'types' and if the AST
  // in 'val' is of the type associated with the tag. Returns 'false' otherwise
  // with a reason in '*err'.
  //   - Requires 'err' to be non-null.
bool IsTyped(const Types& types, const TaggedAST& val, string* err);

}  // namespace type
}  // namespace ast
}  // namespace tervuren

#endif  // LOGLE_TYPE_CHECKER_H_
