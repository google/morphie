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

#include "type_checker.h"

#include <utility>

#include "base/string.h"
#include "gtest.h"

namespace logle {
namespace ast {
namespace type {
namespace {

// Test that all methods crash if the input error string pointer is null.
//
// IsType(..) should crash if given a null pointer as input.
TEST(TypeCheckerDeathTest, NullErrorToIsType) {
  AST ast;
  EXPECT_DEATH({ IsType(ast, nullptr); }, ".*");
}

// AreTypes(..) should crash if given a null pointer as input.
TEST(TypeCheckerDeathTest, NullErrorToAreTypes) {
  Types types;
  EXPECT_DEATH({ AreTypes(types, nullptr); }, ".*");
}

class TypeCheckerTest : public ::testing::Test {
 protected:
  TypeCheckerTest() {}

  virtual void MakeCompositeType(Operator op) {
    type_.mutable_c_ast()->set_op(op);
    type_.mutable_c_ast()->clear_arg();
    type_.set_name("foo");
    type_.set_is_nullable(true);
  }

  virtual void MakeCompositeTypes(const string& tag, Operator op,
                                  PrimitiveType ptype) {
    MakeCompositeType(op);
    AST* arg = type_.mutable_c_ast()->add_arg();
    arg->set_name("bar");
    arg->set_is_nullable(true);
    arg->mutable_p_ast()->set_type(ptype);
    types_.insert({tag, type_});
  }

  virtual void MakeNullableType(const string& tag, PrimitiveType ptype) {
    type_.mutable_p_ast()->set_type(ptype);
    type_.set_name("foo");
    type_.set_is_nullable(true);
    types_.insert({tag, type_});
  }

  // Create an AST with the given operator as the root and the number of
  // specified INT arguments.
  virtual void SetUpCompositeData(Operator op, int num_args) {
    tag_ = "Tag";
    MakeCompositeTypes(tag_, op, PrimitiveType::INT);
    data_.set_tag(tag_);
    data_.mutable_ast()->clear_name();
    data_.mutable_ast()->clear_is_nullable();
    data_.mutable_ast()->mutable_c_ast()->set_op(op);
    data_.mutable_ast()->mutable_c_ast()->clear_arg();
    AST* arg;
    for (int i = 0; i < num_args; ++i) {
      arg = data_.mutable_ast()->mutable_c_ast()->add_arg();
      arg->mutable_p_ast()->set_type(PrimitiveType::INT);
    }
  }

  Types types_;
  string tag_;
  string err_;
  AST type_;
  TaggedAST data_;
};
// The family of tests below apply to methods to check if an AST is a type.
//
// Reject an AST for the null type if the 'name' and 'is_nullable' fields are
// missing or of 'is_nullable' is false.
TEST_F(TypeCheckerTest, NullType) {
  type_.clear_name();
  type_.clear_is_nullable();
  type_.set_allocated_p_ast(nullptr);
  EXPECT_FALSE(IsType(type_, &err_));
  type_.set_name("foo");
  EXPECT_FALSE(IsType(type_, &err_));
  type_.clear_name();
  type_.set_is_nullable(true);
  EXPECT_FALSE(IsType(type_, &err_));
  type_.set_name("foo");
  EXPECT_TRUE(IsType(type_, &err_));
  type_.set_is_nullable(false);
  EXPECT_FALSE(IsType(type_, &err_));
}

// Test validation of primitive types.
// Reject primitive types that have no 'name' and no 'is_nullable' fields.
TEST_F(TypeCheckerTest, NamelessNulllessPrimitiveTypes) {
  type_.clear_name();
  type_.clear_is_nullable();
  type_.mutable_p_ast()->set_type(PrimitiveType::BOOL);
  EXPECT_FALSE(IsType(type_, &err_));
  type_.mutable_p_ast()->set_type(PrimitiveType::INT);
  EXPECT_FALSE(IsType(type_, &err_));
  type_.mutable_p_ast()->set_type(PrimitiveType::STRING);
  EXPECT_FALSE(IsType(type_, &err_));
  type_.mutable_p_ast()->set_type(PrimitiveType::TIMESTAMP);
  EXPECT_FALSE(IsType(type_, &err_));
}

// Reject primitive types with a 'name' but no 'is_nullable' field.
TEST_F(TypeCheckerTest, NamedNulllessPrimitiveTypes) {
  type_.set_name("foo");
  type_.mutable_p_ast()->set_type(PrimitiveType::BOOL);
  EXPECT_FALSE(IsType(type_, &err_));
  type_.mutable_p_ast()->set_type(PrimitiveType::INT);
  EXPECT_FALSE(IsType(type_, &err_));
  type_.mutable_p_ast()->set_type(PrimitiveType::STRING);
  EXPECT_FALSE(IsType(type_, &err_));
  type_.mutable_p_ast()->set_type(PrimitiveType::TIMESTAMP);
  EXPECT_FALSE(IsType(type_, &err_));
}

// Reject primitive types with the 'is_nullable' but not 'name' field.
TEST_F(TypeCheckerTest, NamelessNullablePrimitiveTypes) {
  type_.clear_name();
  type_.set_is_nullable(true);
  type_.mutable_p_ast()->set_type(PrimitiveType::BOOL);
  EXPECT_FALSE(IsType(type_, &err_));
  type_.mutable_p_ast()->set_type(PrimitiveType::INT);
  EXPECT_FALSE(IsType(type_, &err_));
  type_.mutable_p_ast()->set_type(PrimitiveType::STRING);
  EXPECT_FALSE(IsType(type_, &err_));
  type_.mutable_p_ast()->set_type(PrimitiveType::TIMESTAMP);
  EXPECT_FALSE(IsType(type_, &err_));
}

// Accept primitive types with 'name' and 'is_nullable' fields.
TEST_F(TypeCheckerTest, AcceptablePrimitiveTypes) {
  type_.set_name("foo");
  type_.set_is_nullable(true);
  type_.mutable_p_ast()->set_type(PrimitiveType::BOOL);
  EXPECT_TRUE(IsType(type_, &err_));
  type_.mutable_p_ast()->set_type(PrimitiveType::INT);
  EXPECT_TRUE(IsType(type_, &err_));
  type_.mutable_p_ast()->set_type(PrimitiveType::STRING);
  EXPECT_TRUE(IsType(type_, &err_));
  type_.mutable_p_ast()->set_type(PrimitiveType::TIMESTAMP);
  EXPECT_TRUE(IsType(type_, &err_));
}

// Test validation of interval types.
// An interval with no arguments is not a type no matter how 'name' and
// 'is_nullable' are set.
TEST_F(TypeCheckerTest, IntervalWithNoArguments) {
  MakeCompositeType(Operator::INTERVAL);
  type_.clear_name();
  type_.clear_is_nullable();
  EXPECT_FALSE(IsType(type_, &err_));
  type_.set_name("foo");
  EXPECT_FALSE(IsType(type_, &err_));
  type_.clear_name();
  type_.set_is_nullable(true);
  EXPECT_FALSE(IsType(type_, &err_));
  type_.set_name("foo");
  EXPECT_FALSE(IsType(type_, &err_));
}

// The interval type constructor must have a primitive type argument.
TEST_F(TypeCheckerTest, IntervalWithPrimitiveArguments) {
  MakeCompositeType(Operator::INTERVAL);
  AST* arg = type_.mutable_c_ast()->add_arg();
  // An interval with a non-type AST argument is not a type.
  arg->set_name("bar");
  arg->set_is_nullable(false);
  EXPECT_FALSE(IsType(type_, &err_));
  // An interval with a primitive type argument is a type.
  arg->mutable_p_ast()->set_type(PrimitiveType::BOOL);
  EXPECT_TRUE(IsType(type_, &err_));
  arg->mutable_p_ast()->set_type(PrimitiveType::INT);
  EXPECT_TRUE(IsType(type_, &err_));
  arg->mutable_p_ast()->set_type(PrimitiveType::STRING);
  EXPECT_TRUE(IsType(type_, &err_));
  arg->mutable_p_ast()->set_type(PrimitiveType::TIMESTAMP);
  EXPECT_TRUE(IsType(type_, &err_));
}

// The interval type constructor must have exactly one primitive type argument.
TEST_F(TypeCheckerTest, IntervalWithTwoPrimitiveArguments) {
  MakeCompositeType(Operator::INTERVAL);
  AST* arg = type_.mutable_c_ast()->add_arg();
  arg->set_name("bar");
  arg->set_is_nullable(false);
  arg->mutable_p_ast()->set_type(PrimitiveType::INT);
  EXPECT_TRUE(IsType(type_, &err_));
  arg = type_.mutable_c_ast()->add_arg();
  arg->set_name("baz");
  arg->set_is_nullable(false);
  arg->mutable_p_ast()->set_type(PrimitiveType::INT);
  EXPECT_FALSE(IsType(type_, &err_));
}

// Intervals cannot be defined over composite types.
TEST_F(TypeCheckerTest, IntervalWithCompositeArgument) {
  MakeCompositeType(Operator::INTERVAL);
  AST* arg = type_.mutable_c_ast()->add_arg();
  arg->set_name("bar");
  arg->set_is_nullable(false);
  arg->mutable_c_ast()->set_op(Operator::SET);
  AST* arg2 = arg->mutable_c_ast()->add_arg();
  arg2->set_name("baz");
  arg2->set_is_nullable(false);
  arg2->mutable_p_ast()->set_type(PrimitiveType::INT);
  EXPECT_TRUE(IsType(*arg, &err_));
  EXPECT_FALSE(IsType(type_, &err_));
}

// Test validation of ASTs representing composite types.
// Reject ASTs with List, Tuple and Set constructors that have no argument.
TEST_F(TypeCheckerTest, ContainerWithNoArguments) {
  MakeCompositeType(Operator::LIST);
  EXPECT_FALSE(IsType(type_, &err_));
  MakeCompositeType(Operator::TUPLE);
  EXPECT_FALSE(IsType(type_, &err_));
  MakeCompositeType(Operator::SET);
  EXPECT_FALSE(IsType(type_, &err_));
}

// Accept List, Tuple and Set constructors with a single primitive argument.
TEST_F(TypeCheckerTest, ContainerWithOneArgument) {
  MakeCompositeType(Operator::LIST);
  AST* arg = type_.mutable_c_ast()->add_arg();
  arg->set_name("bar");
  arg->set_is_nullable(false);
  arg->mutable_p_ast()->set_type(PrimitiveType::INT);
  EXPECT_TRUE(IsType(type_, &err_));
  type_.mutable_c_ast()->set_op(Operator::TUPLE);
  EXPECT_TRUE(IsType(type_, &err_));
  type_.mutable_c_ast()->set_op(Operator::SET);
  EXPECT_TRUE(IsType(type_, &err_));
}

// Reject List and Set constructors with multiple arguments but not Tuple.
TEST_F(TypeCheckerTest, ContainerWithTwoArgument) {
  MakeCompositeType(Operator::LIST);
  AST* arg = type_.mutable_c_ast()->add_arg();
  arg->set_name("bar");
  arg->set_is_nullable(false);
  arg->mutable_p_ast()->set_type(PrimitiveType::INT);
  arg = type_.mutable_c_ast()->add_arg();
  arg->set_name("baz");
  arg->set_is_nullable(false);
  arg->mutable_p_ast()->set_type(PrimitiveType::INT);
  EXPECT_FALSE(IsType(type_, &err_));
  type_.mutable_c_ast()->set_op(Operator::TUPLE);
  EXPECT_TRUE(IsType(type_, &err_));
  type_.mutable_c_ast()->set_op(Operator::SET);
  EXPECT_FALSE(IsType(type_, &err_));
}

// Check if all ASTs in a map represent types. The function MakeNullableType
// adds an entry to the map 'types_'.
TEST_F(TypeCheckerTest, MultipleTypes) {
  MakeNullableType("Int-AST", PrimitiveType::INT);
  EXPECT_TRUE(AreTypes(types_, &err_));
  MakeNullableType("Bool-AST", PrimitiveType::BOOL);
  EXPECT_TRUE(AreTypes(types_, &err_));
  type_.clear_name();
  type_.set_is_nullable(true);
  type_.mutable_p_ast()->set_type(PrimitiveType::INT);
  types_.insert({"Non-type", type_});
  EXPECT_FALSE(AreTypes(types_, &err_));
}

// The family of tests below determine correctness of type checking.
//
// Crash if a null pointer is provided as input to the IsTyped methods.
TEST(TypeCheckerDeathTest, NullErrorToIsTypedAST) {
  AST type, val;
  EXPECT_DEATH({ IsTyped(type, val, nullptr); }, ".*");
}

TEST(TypeCheckerDeathTest, NonTypeToIsTypedAST) {
  AST type, val;
  string err;
  EXPECT_DEATH({ IsTyped(type, val, &err); }, ".*");
}

TEST(TypeCheckerDeathTest, NullErrorToIsTyped) {
  Types types;
  TaggedAST data;
  EXPECT_DEATH({ IsTyped(types, data, nullptr); }, ".*");
}

TEST(TypeCheckerDeathTest, NonTypeToIsTyped) {
  Types types;
  TaggedAST data;
  string err;
  data.set_tag("Non-type");
  types.insert({data.tag(), AST()});
  EXPECT_DEATH({ IsTyped(types, data, &err); }, ".*");
}

// Reject TaggedAST values whose tag is not associated with a type.
TEST_F(TypeCheckerTest, MissingTypes) {
  tag_ = "Event";
  data_.set_tag(tag_);
  data_.clear_ast();
  EXPECT_FALSE(IsTyped(types_, data_, &err_));
  MakeNullableType(tag_, PrimitiveType::INT);
  EXPECT_TRUE(IsTyped(types_, data_, &err_));
  data_.set_tag("Resource");
  EXPECT_FALSE(IsTyped(types_, data_, &err_));
}

// Accept only null values for the null type. There are two representations of
// nulll values to check.
TEST_F(TypeCheckerTest, AcceptsNullValueForNullType) {
  tag_ = "Event";
  MakeNullableType(tag_, PrimitiveType::INT);
  types_[tag_].set_allocated_p_ast(nullptr);
  data_.set_tag(tag_);
  data_.clear_ast();
  EXPECT_TRUE(IsTyped(types_, data_, &err_));
  data_.mutable_ast()->mutable_p_ast()->set_type(PrimitiveType::INT);
  data_.mutable_ast()->mutable_p_ast()->clear_val();
  EXPECT_FALSE(IsTyped(types_, data_, &err_));
  data_.mutable_ast()->set_allocated_p_ast(nullptr);
  EXPECT_TRUE(IsTyped(types_, data_, &err_));
}

// Accept ASTs with missing primitive values if the type is nullable. There are
// multiple representations of null values. The 'ast' field of a TaggedAST could
// be missing, or the AST could be present but only have type information.
TEST_F(TypeCheckerTest, ChecksPrimitiveNullness) {
  tag_ = "Event";
  MakeNullableType(tag_, PrimitiveType::INT);
  data_.set_tag(tag_);
  data_.clear_ast();
  EXPECT_TRUE(IsTyped(types_, data_, &err_));
  types_[tag_].set_is_nullable(false);
  EXPECT_FALSE(IsTyped(types_, data_, &err_));
  data_.mutable_ast()->clear_name();
  data_.mutable_ast()->mutable_p_ast()->set_type(PrimitiveType::INT);
  data_.mutable_ast()->mutable_p_ast()->clear_val();
  EXPECT_FALSE(IsTyped(types_, data_, &err_));
  types_[tag_].set_is_nullable(true);
  EXPECT_TRUE(IsTyped(types_, data_, &err_));
}

// Accept/reject the appropriate primitive values.
//
// BOOL values should have BOOL type and the 'bool_val' field set.
TEST_F(TypeCheckerTest, PrimitiveBoolValues) {
  tag_ = "Event";
  MakeNullableType(tag_, PrimitiveType::BOOL);
  data_.set_tag(tag_);
  data_.mutable_ast()->clear_name();
  // Check 'bool_val' set with various types.
  data_.mutable_ast()->mutable_p_ast()->set_type(PrimitiveType::BOOL);
  data_.mutable_ast()->mutable_p_ast()->mutable_val()->set_bool_val(true);
  EXPECT_TRUE(IsTyped(types_, data_, &err_));
  data_.mutable_ast()->mutable_p_ast()->mutable_val()->set_bool_val(false);
  EXPECT_TRUE(IsTyped(types_, data_, &err_));
  data_.mutable_ast()->mutable_p_ast()->set_type(PrimitiveType::INT);
  EXPECT_FALSE(IsTyped(types_, data_, &err_));
  data_.mutable_ast()->mutable_p_ast()->set_type(PrimitiveType::STRING);
  EXPECT_FALSE(IsTyped(types_, data_, &err_));
  data_.mutable_ast()->mutable_p_ast()->set_type(PrimitiveType::TIMESTAMP);
  EXPECT_FALSE(IsTyped(types_, data_, &err_));
  // Check type set to BOOL with various values.
  data_.mutable_ast()->mutable_p_ast()->set_type(PrimitiveType::BOOL);
  data_.mutable_ast()->mutable_p_ast()->mutable_val()->set_int_val(0);
  EXPECT_FALSE(IsTyped(types_, data_, &err_));
  data_.mutable_ast()->mutable_p_ast()->mutable_val()->set_string_val("true");
  EXPECT_FALSE(IsTyped(types_, data_, &err_));
  data_.mutable_ast()->mutable_p_ast()->mutable_val()->set_time_val(1);
  EXPECT_FALSE(IsTyped(types_, data_, &err_));
}

// INT values should have INT type and the 'int_val' field set.
TEST_F(TypeCheckerTest, PrimitiveIntValues) {
  tag_ = "Event";
  MakeNullableType(tag_, PrimitiveType::INT);
  data_.set_tag(tag_);
  data_.mutable_ast()->clear_name();
  // Check 'int_val' set with various types. The BOOL case was tested in
  // PrimitiveBoolValues.
  data_.mutable_ast()->mutable_p_ast()->mutable_val()->set_int_val(0);
  data_.mutable_ast()->mutable_p_ast()->set_type(PrimitiveType::STRING);
  EXPECT_FALSE(IsTyped(types_, data_, &err_));
  data_.mutable_ast()->mutable_p_ast()->set_type(PrimitiveType::TIMESTAMP);
  EXPECT_FALSE(IsTyped(types_, data_, &err_));
  data_.mutable_ast()->mutable_p_ast()->set_type(PrimitiveType::INT);
  EXPECT_TRUE(IsTyped(types_, data_, &err_));
  // Check type set to INT with various values. The BOOL case was tested in
  // PrimitiveBoolValues.
  data_.mutable_ast()->mutable_p_ast()->mutable_val()->set_int_val(-1);
  EXPECT_TRUE(IsTyped(types_, data_, &err_));
  data_.mutable_ast()->mutable_p_ast()->mutable_val()->set_int_val(-0);
  EXPECT_TRUE(IsTyped(types_, data_, &err_));
  data_.mutable_ast()->mutable_p_ast()->mutable_val()->set_string_val("a");
  EXPECT_FALSE(IsTyped(types_, data_, &err_));
  data_.mutable_ast()->mutable_p_ast()->mutable_val()->set_string_val("-a");
  EXPECT_FALSE(IsTyped(types_, data_, &err_));
  data_.mutable_ast()->mutable_p_ast()->mutable_val()->set_string_val("-0a");
  EXPECT_FALSE(IsTyped(types_, data_, &err_));
  data_.mutable_ast()->mutable_p_ast()->mutable_val()->set_time_val(2);
  EXPECT_FALSE(IsTyped(types_, data_, &err_));
}

// STRING values should have STRING type and the 'string_val' field set.
TEST_F(TypeCheckerTest, PrimitiveStringValues) {
  tag_ = "Event";
  MakeNullableType(tag_, PrimitiveType::STRING);
  data_.set_tag(tag_);
  data_.mutable_ast()->clear_name();
  data_.mutable_ast()->mutable_p_ast()->set_type(PrimitiveType::STRING);
  data_.mutable_ast()->mutable_p_ast()->mutable_val()->set_string_val("");
  EXPECT_TRUE(IsTyped(types_, data_, &err_));
  data_.mutable_ast()->mutable_p_ast()->mutable_val()->set_string_val("a");
  EXPECT_TRUE(IsTyped(types_, data_, &err_));
  data_.mutable_ast()->mutable_p_ast()->mutable_val()->set_string_val("0");
  EXPECT_TRUE(IsTyped(types_, data_, &err_));
  data_.mutable_ast()->mutable_p_ast()->mutable_val()->set_string_val("false");
  // Check 'string_val' set with the TIMESTAMP type.
  data_.mutable_ast()->mutable_p_ast()->set_type(PrimitiveType::TIMESTAMP);
  EXPECT_FALSE(IsTyped(types_, data_, &err_));
}

// TIMESTAMP values should have TIMESTAMP type and the 'time_val' field set.
// Only the legal case is tested because other combinations were tested above.
TEST_F(TypeCheckerTest, PrimitiveTimestampValues) {
  tag_ = "Event";
  MakeNullableType(tag_, PrimitiveType::TIMESTAMP);
  data_.set_tag(tag_);
  data_.mutable_ast()->clear_name();
  data_.mutable_ast()->mutable_p_ast()->set_type(PrimitiveType::TIMESTAMP);
  data_.mutable_ast()->mutable_p_ast()->mutable_val()->set_time_val(5120);
  EXPECT_TRUE(IsTyped(types_, data_, &err_));
}

// Test type checking involving composite types and values.
//
// A composite value cannot match a primitive type.
TEST_F(TypeCheckerTest, PrimitiveTypeCompositeValue) {
  tag_ = "Primitive";
  MakeNullableType(tag_, PrimitiveType::INT);
  data_.set_tag(tag_);
  data_.mutable_ast()->mutable_c_ast()->set_op(Operator::SET);
  EXPECT_FALSE(IsTyped(types_[tag_], data_.ast(), &err_));
  EXPECT_FALSE(IsTyped(types_, data_, &err_));
}

// A primitive value cannot match a composite type.
TEST_F(TypeCheckerTest, CompositeTypePrimitiveValue) {
  tag_ = "Composite";
  MakeCompositeTypes(tag_, Operator::SET, PrimitiveType::INT);
  data_.set_tag(tag_);
  data_.mutable_ast()->clear_name();
  data_.mutable_ast()->mutable_p_ast()->set_type(PrimitiveType::INT);
  EXPECT_FALSE(IsTyped(types_[tag_], data_.ast(), &err_));
  EXPECT_FALSE(IsTyped(types_, data_, &err_));
}

// Check type mismatch between composite types and values.
TEST_F(TypeCheckerTest, CompositeTypeCompositeValue) {
  tag_ = "Composite";
  MakeCompositeTypes(tag_, Operator::SET, PrimitiveType::INT);
  data_.set_tag(tag_);
  data_.mutable_ast()->clear_name();
  data_.mutable_ast()->mutable_c_ast()->set_op(Operator::LIST);
  EXPECT_FALSE(IsTyped(types_[tag_], data_.ast(), &err_));
  EXPECT_FALSE(IsTyped(types_, data_, &err_));
}

// Test interval type checking.
//
// Values with no arguments can only be of nullable interval type. There are two
// representations of a null interval to check.
TEST_F(TypeCheckerTest, NullableIntervalValues) {
  SetUpCompositeData(Operator::INTERVAL, 0);
  // Type check a TaggedAST with an 'ast' field that has no arguments.
  types_[tag_].set_is_nullable(true);
  EXPECT_TRUE(IsTyped(types_, data_, &err_));
  types_[tag_].set_is_nullable(false);
  EXPECT_FALSE(IsTyped(types_, data_, &err_));
  // Check a TaggedAST with no 'ast' field.
  data_.clear_ast();
  EXPECT_FALSE(IsTyped(types_, data_, &err_));
  types_[tag_].set_is_nullable(true);
  EXPECT_TRUE(IsTyped(types_, data_, &err_));
}

// An interval value must have zero or two bounds specified.
TEST_F(TypeCheckerTest, ZeroOrTwoIntervalBounds) {
  // Reject an interval value with only one bound.
  SetUpCompositeData(Operator::INTERVAL, 1);
  EXPECT_FALSE(IsTyped(types_, data_, &err_));
  // Accept an interval value with two bounds.
  AST* arg = data_.mutable_ast()->mutable_c_ast()->add_arg();
  arg->mutable_p_ast()->set_type(PrimitiveType::INT);
  arg->mutable_p_ast()->clear_val();
  EXPECT_TRUE(IsTyped(types_, data_, &err_));
  // Reject an interval value with three bounds.
  arg = data_.mutable_ast()->mutable_c_ast()->add_arg();
  arg->mutable_p_ast()->set_type(PrimitiveType::INT);
  arg->mutable_p_ast()->clear_val();
  EXPECT_FALSE(IsTyped(types_, data_, &err_));
}

// All the bounds in an interval must have the same type.
TEST_F(TypeCheckerTest, MultiTypeIntervalBounds) {
  SetUpCompositeData(Operator::INTERVAL, 2);
  // The lower bound is of type INT. Vary the type of the upper bound.
  AST* arg = data_.mutable_ast()->mutable_c_ast()->mutable_arg(1);
  arg->mutable_p_ast()->set_type(PrimitiveType::BOOL);
  EXPECT_FALSE(IsTyped(types_, data_, &err_));
  arg->mutable_p_ast()->set_type(PrimitiveType::STRING);
  EXPECT_FALSE(IsTyped(types_, data_, &err_));
  arg->mutable_p_ast()->set_type(PrimitiveType::TIMESTAMP);
  EXPECT_FALSE(IsTyped(types_, data_, &err_));
}

// The values of the interval bounds should not affect typing.
TEST_F(TypeCheckerTest, DefinedIntervalBounds) {
  SetUpCompositeData(Operator::INTERVAL, 2);
  // The interval [0, null], representing [0, MAX] is of type interval(int).
  AST* arg = data_.mutable_ast()->mutable_c_ast()->mutable_arg(0);
  arg->mutable_p_ast()->mutable_val()->set_int_val(0);
  // The intervals [0,1] and [0,-1] are both of type interval(int).
  arg = data_.mutable_ast()->mutable_c_ast()->mutable_arg(1);
  EXPECT_TRUE(IsTyped(types_, data_, &err_));
  arg->mutable_p_ast()->mutable_val()->set_int_val(1);
  EXPECT_TRUE(IsTyped(types_, data_, &err_));
  arg->mutable_p_ast()->mutable_val()->set_int_val(-1);
  EXPECT_TRUE(IsTyped(types_, data_, &err_));
  // The interval [null,0], representing [MIN, 0] is of type interval(int).
  arg = data_.mutable_ast()->mutable_c_ast()->mutable_arg(0);
  arg->mutable_p_ast()->clear_val();
  EXPECT_TRUE(IsTyped(types_, data_, &err_));
}

// Test type checking of lists, sets and tuples.
//
// Accept empty lists if the type is nullable.
TEST_F(TypeCheckerTest, NullableList) {
  SetUpCompositeData(Operator::LIST, 0);
  // Type check a TaggedAST with an 'ast' field that has no arguments.
  types_[tag_].set_is_nullable(true);
  EXPECT_TRUE(IsTyped(types_, data_, &err_));
  types_[tag_].set_is_nullable(false);
  EXPECT_FALSE(IsTyped(types_, data_, &err_));
}

// Reject lists whose contents have the wrong type.
TEST_F(TypeCheckerTest, MistypedListContents) {
  // Accept the value list(int(null)).
  SetUpCompositeData(Operator::LIST, 1);
  EXPECT_TRUE(IsTyped(types_, data_, &err_));
  // Reject the value list(bool(null)).
  AST* arg = data_.mutable_ast()->mutable_c_ast()->mutable_arg(0);
  arg->mutable_p_ast()->set_type(PrimitiveType::BOOL);
  arg->mutable_p_ast()->mutable_val()->set_bool_val(true);
  EXPECT_FALSE(IsTyped(types_, data_, &err_));
  // Reject the value list(bool(null), int(null)).
  arg = data_.mutable_ast()->mutable_c_ast()->add_arg();
  arg->mutable_p_ast()->set_type(PrimitiveType::INT);
  EXPECT_FALSE(IsTyped(types_, data_, &err_));
  // Accept the value list(int(null), int(null)).
  arg = data_.mutable_ast()->mutable_c_ast()->mutable_arg(0);
  arg->mutable_p_ast()->set_type(PrimitiveType::INT);
  arg->mutable_p_ast()->clear_val();
  EXPECT_TRUE(IsTyped(types_, data_, &err_));
}

// Accept empty sets if the type is nullable.
TEST_F(TypeCheckerTest, NullableSet) {
  SetUpCompositeData(Operator::SET, 0);
  // Type check a TaggedAST with an 'ast' field that has no arguments.
  types_[tag_].set_is_nullable(true);
  EXPECT_TRUE(IsTyped(types_, data_, &err_));
  types_[tag_].set_is_nullable(false);
  EXPECT_FALSE(IsTyped(types_, data_, &err_));
}

// Reject sets whose contents have the wrong type.
TEST_F(TypeCheckerTest, MistypedSetContents) {
  // Accept the value set(int(null)).
  SetUpCompositeData(Operator::SET, 1);
  EXPECT_TRUE(IsTyped(types_, data_, &err_));
  // Reject the value set(bool(null)).
  AST* arg = data_.mutable_ast()->mutable_c_ast()->mutable_arg(0);
  arg->mutable_p_ast()->set_type(PrimitiveType::BOOL);
  arg->mutable_p_ast()->mutable_val()->set_bool_val(true);
  EXPECT_FALSE(IsTyped(types_, data_, &err_));
  // Reject the value set(bool(null), int(null)).
  arg = data_.mutable_ast()->mutable_c_ast()->add_arg();
  arg->mutable_p_ast()->set_type(PrimitiveType::INT);
  EXPECT_FALSE(IsTyped(types_, data_, &err_));
  // Accept the value set(int(null), int(null)).
  arg = data_.mutable_ast()->mutable_c_ast()->mutable_arg(0);
  arg->mutable_p_ast()->set_type(PrimitiveType::INT);
  arg->mutable_p_ast()->clear_val();
  EXPECT_TRUE(IsTyped(types_, data_, &err_));
}

// Accept empty tuples if the type is nullable.
TEST_F(TypeCheckerTest, NullableTuple) {
  SetUpCompositeData(Operator::TUPLE, 0);
  // Type check a TaggedAST with an 'ast' field that has no arguments.
  types_[tag_].set_is_nullable(true);
  EXPECT_TRUE(IsTyped(types_, data_, &err_));
  types_[tag_].set_is_nullable(false);
  EXPECT_FALSE(IsTyped(types_, data_, &err_));
}

// Reject tuples whose contents have the wrong type.
TEST_F(TypeCheckerTest, MistypedTupleContents) {
  // Accept the value tuple(int(null)).
  SetUpCompositeData(Operator::TUPLE, 1);
  EXPECT_TRUE(IsTyped(types_, data_, &err_));
  // Reject the value tuple(bool(null)).
  AST* arg = data_.mutable_ast()->mutable_c_ast()->mutable_arg(0);
  arg->mutable_p_ast()->set_type(PrimitiveType::BOOL);
  arg->mutable_p_ast()->mutable_val()->set_bool_val(true);
  EXPECT_FALSE(IsTyped(types_, data_, &err_));
  EXPECT_FALSE(IsTyped(types_, data_, &err_));
  // Reject the value tuple(bool(null), int(null)).
  arg = data_.mutable_ast()->mutable_c_ast()->add_arg();
  arg->mutable_p_ast()->set_type(PrimitiveType::INT);
  EXPECT_FALSE(IsTyped(types_, data_, &err_));
  // Reject the value tuple(int(null), int(null)). A tuple must contain a fixed
  // number of values defined by its type (in this case, one).
  arg = data_.mutable_ast()->mutable_c_ast()->mutable_arg(0);
  arg->mutable_p_ast()->set_type(PrimitiveType::INT);
  arg->mutable_p_ast()->clear_val();
  EXPECT_FALSE(IsTyped(types_, data_, &err_));
}

// Test tuples of two identical and distinct types.
TEST_F(TypeCheckerTest, TypedPair) {
  // The value tuple(int(null), int(null)) has type tuple(int, int).
  SetUpCompositeData(Operator::TUPLE, 2);
  AST* type_arg = types_[tag_].mutable_c_ast()->add_arg();
  type_arg->set_name("second");
  type_arg->set_is_nullable(true);
  type_arg->mutable_p_ast()->set_type(PrimitiveType::INT);
  EXPECT_TRUE(IsTyped(types_, data_, &err_));
  // The value tuple(int(null), bool(null)) does not have type tuple(int, int).
  type_arg->mutable_p_ast()->set_type(PrimitiveType::BOOL);
  EXPECT_FALSE(IsTyped(types_, data_, &err_));
  // The value tuple(int(null), bool(null)) does not have type tuple(int, bool).
  AST* arg = data_.mutable_ast()->mutable_c_ast()->mutable_arg(1);
  arg->mutable_p_ast()->set_type(PrimitiveType::BOOL);
  EXPECT_TRUE(IsTyped(types_, data_, &err_));
}

}  // namespace
}  // namespace type
}  // namespace ast
}  // namespace logle
