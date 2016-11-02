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

// A value in the context of this file is an Abstract Syntax Tree (AST) for a
// value of a type defined in third_party/logle/ast.proto. This file provides
// utilities for constructing and manipulating value ASTs.
//
// Example. This code constructs a value AST representing the empty integer
// intervals [] and the closed unit interval [0,1]. The checks illustrate the
// guarantees that this API provides.
//
// AST type = ast::type::MakeInterval("itv", PrimitiveType::INT));
// AST empty = ast::value::MakeEmptyInterval();
// AST unit = ast::value::MakeIntInterval(0, 1);
// string err;
// CHECK(ast::value::IsValue(val, &err)); // The ASTs 'empty' and 'unit'
// CHECK(ast::value::IsValue(val, &err)); // represent values.
// CHECK(ast::type::IsTyped(type, empty, &err)); // The intervals are of type
// CHECK(ast::type::IsTyped(type, unit, &err));  // interval(int).
#ifndef LOGLE_VALUE_H_
#define LOGLE_VALUE_H_

#include <cstdint>

#include "third_party/logle/graph/ast.pb.h"
#include "base/string.h"

namespace morphie {
namespace ast {
namespace value {

// The Value class contains functions named Make[Type](...), which return a
// value AST of the specified type. These ASTs satisfy the well-formedness
// constraints on ASTs defined in third_party/logle/value_checker.h and the
// typing constraints defined in third_party/logle/type_checker.h.

// Generate the null value. Unlike the AST for the null type, the AST for a
// null value has no 'name' or 'is_nullable' fields.
AST MakeNull();

// The functions below construct primitive values. A primitive null value has
// a type but no value.
AST MakePrimitiveNull(PrimitiveType ptype);
AST MakeBool(bool val);
AST MakeInt(int val);
AST MakeString(const string& val);
AST MakeTimestampFromUnixMicros(int64_t val);

// Constructs a timestamp AST from an RFC3339 timestamp. Returns a null
// timestamp if the argument 'val' has the wrong format.
AST MakeTimestampFromRFC3339(const string& val);

// The functions below return the value represented by a AST of primitive
// type. The function argument must be of the appropriate type.
bool GetBool(const AST& val);
int GetInt(const AST& val);
string GetString(const AST& val);
int GetTimestamp(const AST& val);

// The functions below construct composite values. A null composite type has
// an operator but no arguments.
AST MakeCompositeNull(Operator op);

// There are functions to construct four kinds of intervals.
// - [] : the empty interval that contains no values.
// - [lower_val, MAX] or [MIN, upper_val] : the half-intervals that have
//   either a lower bound or an upper bound but not both.
// - [lower_val, upper_val] : an interval with both bounds.
// - [MIN, MAX] : the max interval that covers all values of a type.
//
// Returns the empty interval. It contains no type information.
AST MakeEmptyInterval(PrimitiveType ptype);

// Returns an interval covering the range of values of a primitive type.
AST MakeMaxInterval(PrimitiveType ptype);

// The Make[Type]Interval(upper_val, lower_val) functions return an interval of
// the named type with an upper bound and a lower bound. The functions
// Make[Type]HalfInterval(val, is_lower) return an interval with 'val' as a
// lower bound if 'is_lower' is true and with 'val' as the upper bound
// otherwise.
AST MakeBoolInterval(bool lower_val, bool upper_val);
AST MakeBoolHalfInterval(bool val, bool is_lower);
AST MakeIntInterval(int lower_val, int upper_val);
AST MakeIntHalfInterval(int val, bool is_lower);
AST MakeStringInterval(const string& lower_val, const string& upper_val);
AST MakeStringHalfInterval(const string& val, bool is_lower);
AST MakeTimestampInterval(int lower_val, int upper_val);
AST MakeTimestampHalfInterval(int val, bool is_lower);

// Methods to construct, manipulate and query containers follow. The Append(),
// Insert() and SetField() functions validate their inputs with time
// complexity that is that is linear in the size of all the input ASTs.
//
// Return the number of elements in a container. Requires that 'container' is
// a list, a set or a tuple.
int Size(const AST& container);

// Return an empty list. Complexity: constant time.
AST MakeEmptyList();

// Append 'arg' to the end of the '*list'. Requires that
// - 'type' is a type of the form list(arg_type), where 'arg_type' is a type.
// - 'arg' is a value of type 'arg_type'.
// - 'list' is not null and '*list' is a value of type 'type'.
// Complexity: constant time.
void Append(const AST& type, const AST& arg, AST* list);

// Return an empty set. Complexity: constant time.
AST MakeEmptySet();

// Insert 'arg' into '*set'. Requires that
// - 'type' is a type of the form set(arg_type), where 'arg_type' is a type.
// - 'arg' is a value of type 'arg_type'.
// - 'set' is not null and '*set' is a value of type 'type'.
// Complexity: linear time in the number of set elements and size of these
// elements. This method checks if an element is already in '*set' by
// traversing and canonicalizing set elements. Since these sets are intended
// to be extremely small, a naive implementation is used for now.
void Insert(const AST& type, const AST& arg, AST* set);

// Return a tuple with 'num_fields' uninitialized fields. Complexity: constant
// time.
AST MakeNullTuple(int num_fields);

// Set the 'field_num' field to 'arg'. Requires that
// - 'type' is a type of the form tuple(arg_type), where 'arg_type' is a type.
// - 'field_num' is at least 0 and strictly less than value::Size(type).
// - 'tuple' is not null and '*tuple' is a tuple.
// Unlike the functions for manipulating lists and sets, '*tuple' is not
// required to be a value of type 'type' because the fields in '*tuple' may
// not be initialized.
void SetField(const AST& type, int field_num, const AST& arg, AST* tuple);

}  // namespace value
}  // namespace ast
}  // namespace morphie

#endif  // LOGLE_VALUE_H_
