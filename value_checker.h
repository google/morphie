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

// A value is an Abstract Syntax Tree (AST) representing an instance of a type
// in the grammar given in ast.proto.
//
//   Eg. In the pretty printing notation for ASTs, list(int) is the AST for a
//   type and list(int(5), int(3)) is the AST for a value of that type.
//
// Some values have multiple, non-identical representations which have to be
// accounted for when checking if two values are the same.
#ifndef THIRD_PARTY_LOGLE_VALUE_CHECKER_H_
#define THIRD_PARTY_LOGLE_VALUE_CHECKER_H_

#include "third_party/logle/ast.pb.h"
#include "third_party/logle/base/string.h"

namespace tervuren {
namespace ast {
namespace value {

// This file contains functions  for ensuring that an AST is a value, for
// checking if two value ASTs have the same structure and for  transforming
// value ASTs to canonical form.
//
// A primitive value 'val' is of primitive type 'ptype' if the type and value
// in 'val' match that of 'ptype'. If ptype is PrimitiveType::BOOL, then
// val.type() should also be PrimitiveType::BOOL and val.val().bool_val()
// should be set.
//
// This function returns true if 'val' has the type 'ptype'.
bool IsPrimitive(const PrimitiveType ptype, const PrimitiveValue& val);

// An AST 'ast' represents a value if one of these conditions holds.
// - The AST represents the null value and has no fields set.
//
// - The AST has a primitive AST whose value respects the declared type.
//
// - The AST is an interval if it has no arguments (in which case it
//   represents the empty interval), or if it has two arguments, both of
//   which are possibly null values of the same type. The intervals are
//   closed and can omit one or both bounds.
//
//   Eg. These are all interval values of type interval?(int?).
//   a. interval(null) is the empty interval.
//   b. interval(int(null), int(0)) encodes ints less than or equal to 0.
//   c. interval(int(0), int(null)) encodes ints greater than or equal to 0.
//   d. interval(int(0), int(0)) encodes the value 0.
//   e. interval(int(0), int(2)) encodes the values 0, 1 and 2.
//   f. interval(int(null), int(null)) encodes all values.
//
// - The AST is a list or a set if all its arguments are values and have the
//   same type. Checking if all the values are the same type is problematic
//   because the types are nullable and an AST representing a value does not
//   carry all the required type information. For example, the values
//   list(tuple(null, int(5))) and list(tuple(string(foo), null)) may both be
//   of type  list(tuple(string,int)). Checking if the values in a set or
//   list have the same type would require some form of type inference.
//
//   This code does not perform type inference for several reasons.
//   1. Values will not be used in isolation but in conjunction with a type
//   declaration, so the type checking can be used to eliminate invalid
//   values even if value checking is imprecise.
//   2. If the type system in ast.proto is extended to include sum types
//   (meaning, types of the form, oneof(t1,t2)), type inference will not help
//   value checking. For example, the AST list(int(5), string(foo)) represents
//   a value of type list(oneof(int,string)).
//   Currently, implementing precise value checking does not seem beneficial.
//   The code will check if every element of a list is an AST representing a
//   value, but will not check if these values have the same type.
//
// - The AST is a tuple if each of its arguments is a value.
bool IsValue(const AST& ast, string* err);

// Return true if val1 is strictly less than val2.
// - Requires that val1 and val2 are not null.
bool LT(const PrimitiveAST& val1, const PrimitiveAST& val2);

// Two value ASTs are isomorphic if they have the same structure and contents.
//
// Eg. Each line below has equivalent but non-isomorphic values.
//   interval(int(3), int(1)), interval(null), interval(int(-1), int(-5))
//   set(int(0), int(1)), set(int(1), int(0))
//
// Formally, two primitive ASTs are isomorphic if they have the same type and
// either both have no value, or both have the same value field.  Composite
// ASTs are isomorphic if they have the same operator, same number of
// arguments, and the arguments are pair-wise isomorphic.
//
// Isomorphic ASTs represent the same value but a value can have multiple,
// non-isomorphic representations. Isomorphism can be checked by traversing an
// AST and is more efficient than checking equality.
//
// This function returns true if its arguments are isomorphic. It requires
// arguments that are values.
bool Isomorphic(const AST& val1, const AST& val2);

// The canonical form for ASTs provides a unique representation of each value
// so that isomorphism can be used to check equality. Value ASTs are not in
// canonical form by default because it is simpler and more efficient to
// implement operations on arbitrary value ASTs. ASTs can be transformed to
// canonical form just before checking isomorphism.
//
// Eg. The equal but non-isomorphic examples used for the Isomorphic() function
// are shown in canonical form here.
//   int("0") is the canonical form for int("00") and int("-0").
//   timestamp("2012-04-03T00:25:21+00:00") is the canonical form for
//   timestamp("2012-04-02T23:25:21-01:00").
//   set(int("0"), int("1")) is the canonical form for
//   set(int("1"), int("0")).
//
// A value AST is in canonical form if:
// - the empty interval is represented as interval(null).
// - LIST and TUPLE contents are in canonical form.
// - SET elements are in canonical form and occur in the lexicographic order
//   of their serialized protobuf representations.
//
// Transforms a value AST to canonical form. Requires that val is not null.
void Canonicalize(AST* val);

}  // namespace value
}  // namespace ast
}  // namespace tervuren

#endif  // THIRD_PARTY_LOGLE_VALUE_CHECKER_H_
