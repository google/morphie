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

#include "third_party/logle/value.h"

#include "third_party/logle/gtest.h"
#include "third_party/logle/type.h"
#include "third_party/logle/type_checker.h"
#include "third_party/logle/value_checker.h"

namespace tervuren {
namespace ast {
namespace value {
namespace {

class ValueTest : public ::testing::Test {
 protected:
  AST type_;
  AST val_;
  string err_;
};

// Test creation of the null value.
TEST_F(ValueTest, CreatesNullValue) {
  type_ = type::MakeNull("foo");
  val_ = MakeNull();
  EXPECT_TRUE(IsValue(val_, &err_));
  EXPECT_TRUE(type::IsTyped(type_, val_, &err_));
}

// Test creation of primitive values. Always check the null value.
//
// Boolean values: null, true and false.
TEST_F(ValueTest, CreatesBools) {
  type_ = type::MakeBool("foo", true);
  val_ = MakePrimitiveNull(PrimitiveType::BOOL);
  EXPECT_TRUE(IsValue(val_, &err_));
  EXPECT_TRUE(type::IsTyped(type_, val_, &err_));
  val_ = MakeBool(true);
  EXPECT_TRUE(IsValue(val_, &err_));
  EXPECT_TRUE(type::IsTyped(type_, val_, &err_));
  EXPECT_EQ(true, GetBool(val_));
  val_ = MakeBool(false);
  EXPECT_TRUE(IsValue(val_, &err_));
  EXPECT_TRUE(type::IsTyped(type_, val_, &err_));
  EXPECT_EQ(false, GetBool(val_));
}

// Integer values: null, a positive and a negative value.
TEST_F(ValueTest, CreatesInts) {
  type_ = type::MakeInt("foo", true);
  val_ = MakePrimitiveNull(PrimitiveType::INT);
  EXPECT_TRUE(IsValue(val_, &err_));
  EXPECT_TRUE(type::IsTyped(type_, val_, &err_));
  val_ = MakeInt(0);
  EXPECT_TRUE(IsValue(val_, &err_));
  EXPECT_TRUE(type::IsTyped(type_, val_, &err_));
  EXPECT_EQ(0, GetInt(val_));
  val_ = MakeInt(-5);
  EXPECT_TRUE(IsValue(val_, &err_));
  EXPECT_TRUE(type::IsTyped(type_, val_, &err_));
  EXPECT_EQ(-5, GetInt(val_));
  val_ = MakeInt(50);
  EXPECT_TRUE(IsValue(val_, &err_));
  EXPECT_TRUE(type::IsTyped(type_, val_, &err_));
  EXPECT_EQ(50, GetInt(val_));
}

// String values: null value, the empty string and a non-empty string.
TEST_F(ValueTest, CreatesStrings) {
  type_ = type::MakeString("foo", true);
  val_ = MakePrimitiveNull(PrimitiveType::STRING);
  EXPECT_TRUE(IsValue(val_, &err_));
  EXPECT_TRUE(type::IsTyped(type_, val_, &err_));
  val_ = MakeString("");
  EXPECT_TRUE(IsValue(val_, &err_));
  EXPECT_TRUE(type::IsTyped(type_, val_, &err_));
  EXPECT_EQ("", GetString(val_));
  val_ = MakeString("bar");
  EXPECT_TRUE(IsValue(val_, &err_));
  EXPECT_TRUE(type::IsTyped(type_, val_, &err_));
  EXPECT_EQ("bar", GetString(val_));
}

// Timestamp values: null value and two non-null timestamps.
TEST_F(ValueTest, CreatesTimestampsFromMicros) {
  type_ = type::MakeTimestamp("foo", true);
  val_ = MakePrimitiveNull(PrimitiveType::TIMESTAMP);
  EXPECT_TRUE(IsValue(val_, &err_));
  EXPECT_TRUE(type::IsTyped(type_, val_, &err_));
  val_ = MakeTimestampFromUnixMicros(0);
  EXPECT_TRUE(IsValue(val_, &err_));
  EXPECT_TRUE(type::IsTyped(type_, val_, &err_));
  EXPECT_EQ(0, GetTimestamp(val_));
  val_ = MakeTimestampFromUnixMicros(-1048576);
  EXPECT_TRUE(IsValue(val_, &err_));
  EXPECT_TRUE(type::IsTyped(type_, val_, &err_));
  EXPECT_EQ(-1048576, GetTimestamp(val_));
}

// Timestamp values: null value, the empty string and a non-empty string.
TEST_F(ValueTest, CreatesTimestampsFromRFC3339) {
  type_ = type::MakeTimestamp("foo", true);
  val_ = MakePrimitiveNull(PrimitiveType::TIMESTAMP);
  EXPECT_TRUE(IsValue(val_, &err_));
  EXPECT_TRUE(type::IsTyped(type_, val_, &err_));
  val_ = MakeTimestampFromRFC3339("2012-04-03T00:25:22+00:00");
  EXPECT_TRUE(IsValue(val_, &err_));
  EXPECT_TRUE(type::IsTyped(type_, val_, &err_));
  // Should return a valid timestamp even if the input is not a timestamp.
  val_ = MakeTimestampFromRFC3339("NOT A TIMESTAMP");
  EXPECT_TRUE(IsValue(val_, &err_));
  EXPECT_TRUE(type::IsTyped(type_, val_, &err_));
}

// Test creation of composite values. Each test checks also the null value.
// Test creation of Bool intervals.
TEST_F(ValueTest, CreatesBoolIntervals) {
  type_ = type::MakeInterval("foo", PrimitiveType::BOOL);
  val_ = MakeEmptyInterval(PrimitiveType::BOOL);
  EXPECT_TRUE(IsValue(val_, &err_));
  EXPECT_TRUE(type::IsTyped(type_, val_, &err_));
  val_ = MakeMaxInterval(PrimitiveType::BOOL);
  EXPECT_TRUE(IsValue(val_, &err_));
  EXPECT_TRUE(type::IsTyped(type_, val_, &err_));
  val_ = MakeBoolInterval(true, true);
  EXPECT_TRUE(IsValue(val_, &err_));
  EXPECT_TRUE(type::IsTyped(type_, val_, &err_));
  val_ = MakeBoolHalfInterval(true, true);
  EXPECT_TRUE(IsValue(val_, &err_));
  EXPECT_TRUE(type::IsTyped(type_, val_, &err_));
}

// Test creation of Int intervals.
TEST_F(ValueTest, CreatesIntIntervals) {
  type_ = type::MakeInterval("foo", PrimitiveType::INT);
  val_ = MakeEmptyInterval(PrimitiveType::INT);
  EXPECT_TRUE(IsValue(val_, &err_));
  EXPECT_TRUE(type::IsTyped(type_, val_, &err_));
  val_ = MakeMaxInterval(PrimitiveType::INT);
  EXPECT_TRUE(IsValue(val_, &err_));
  EXPECT_TRUE(type::IsTyped(type_, val_, &err_));
  val_ = MakeIntInterval(-1, 1);
  EXPECT_TRUE(IsValue(val_, &err_));
  EXPECT_TRUE(type::IsTyped(type_, val_, &err_));
  val_ = MakeIntHalfInterval(5, true);
  EXPECT_TRUE(IsValue(val_, &err_));
  EXPECT_TRUE(type::IsTyped(type_, val_, &err_));
}

// Test creation of String intervals.
TEST_F(ValueTest, CreatesStringIntervals) {
  type_ = type::MakeInterval("foo", PrimitiveType::STRING);
  val_ = MakeEmptyInterval(PrimitiveType::STRING);
  EXPECT_TRUE(IsValue(val_, &err_));
  EXPECT_TRUE(type::IsTyped(type_, val_, &err_));
  val_ = MakeMaxInterval(PrimitiveType::STRING);
  EXPECT_TRUE(IsValue(val_, &err_));
  EXPECT_TRUE(type::IsTyped(type_, val_, &err_));
  val_ = MakeStringInterval("", "bar");
  EXPECT_TRUE(IsValue(val_, &err_));
  EXPECT_TRUE(type::IsTyped(type_, val_, &err_));
  val_ = MakeStringHalfInterval("baz", true);
  EXPECT_TRUE(IsValue(val_, &err_));
  EXPECT_TRUE(type::IsTyped(type_, val_, &err_));
}

// Test creation of Timestamp intervals.
TEST_F(ValueTest, CreatesTimestampIntervals) {
  type_ = type::MakeInterval("foo", PrimitiveType::TIMESTAMP);
  val_ = MakeEmptyInterval(PrimitiveType::TIMESTAMP);
  EXPECT_TRUE(IsValue(val_, &err_));
  EXPECT_TRUE(type::IsTyped(type_, val_, &err_));
  val_ = MakeMaxInterval(PrimitiveType::TIMESTAMP);
  EXPECT_TRUE(IsValue(val_, &err_));
  EXPECT_TRUE(type::IsTyped(type_, val_, &err_));
  val_ = MakeTimestampInterval(-1, 1);
  EXPECT_TRUE(IsValue(val_, &err_));
  EXPECT_TRUE(type::IsTyped(type_, val_, &err_));
  val_ = MakeTimestampHalfInterval(5, true);
  EXPECT_TRUE(IsValue(val_, &err_));
  EXPECT_TRUE(type::IsTyped(type_, val_, &err_));
}

// Test methods that manipulate containers.
// Crashes if the size function is called on a non-container.
TEST(ValueDeathTest, OnlyContainersHaveSize) {
  AST val = MakeBool(false);
  EXPECT_DEATH({ Size(val); }, ".*");
}

// Test fatal input arguments requirements of Append.
// Crashes if a value is passed as a type.
TEST(ValueDeathTest, AppendRequiresAType) {
  AST list = MakeEmptyList();
  AST val = MakeBool(false);
  EXPECT_DEATH({ Append(val, val, &list); }, ".*");
}

// Crashes if a non-list type is passed to Append.
TEST(ValueDeathTest, AppendRequiresAListType) {
  AST list = MakeEmptyList();
  AST val = MakeBool(false);
  AST type = type::MakeSet("foo", true, type::MakeBool("Element", true));
  EXPECT_DEATH({ Append(type, val, &list); }, ".*");
}

// Crashes if the list element has the wrong type.
TEST(ValueDeathTest, AppendRequiresTypedArgument) {
  AST list = MakeEmptyList();
  AST val = MakeInt(5);
  AST type = type::MakeList("foo", true, type::MakeBool("Element", true));
  EXPECT_DEATH({ Append(type, val, &list); }, ".*");
}

// Crashes if the list pointer is null.
TEST(ValueDeathTest, AppendRequiresNonNullPointer) {
  AST val = MakeInt(5);
  AST type = type::MakeList("foo", true, type::MakeBool("Element", true));
  EXPECT_DEATH({ Append(type, val, nullptr); }, ".*");
}

// Crashes if the pointer 'list' is to a non-list.
TEST(ValueDeathTest, AppendRequiresList) {
  AST val = MakeInt(5);
  AST set = MakeEmptySet();
  AST type = type::MakeList("foo", true, type::MakeBool("Element", true));
  EXPECT_DEATH({ Append(type, val, &set); }, ".*");
}

// Test list creation and appending.
TEST_F(ValueTest, ConstructsLists) {
  type_ = type::MakeList("foo", true, type::MakeBool("Element", true));
  AST set_type = type::MakeSet("foo", true, type::MakeBool("Element", true));
  // Check that the empty list is a list and has size zero.
  val_ = MakeEmptyList();
  EXPECT_TRUE(IsValue(val_, &err_));
  EXPECT_TRUE(type::IsTyped(type_, val_, &err_));
  EXPECT_FALSE(type::IsTyped(set_type, val_, &err_));
  EXPECT_EQ(Size(val_), 0);
  // After appending, the list has size one.
  Append(type_, MakeBool(true), &val_);
  EXPECT_TRUE(IsValue(val_, &err_));
  EXPECT_TRUE(type::IsTyped(type_, val_, &err_));
  EXPECT_EQ(Size(val_), 1);
  // Two appends lead to size two.
  Append(type_, MakeBool(true), &val_);
  EXPECT_EQ(Size(val_), 2);
}

// Test fatal input arguments requirements of Insert.
// Crashes if a value is passed as a type.
TEST(ValueDeathTest, InsertRequiresAType) {
  AST set = MakeEmptySet();
  AST val = MakeBool(false);
  EXPECT_DEATH({ Insert(set, val, &set); }, ".*");
}

// Crashes if a non-set type is passed to Insert.
TEST(ValueDeathTest, InsertRequiresASetType) {
  AST set = MakeEmptySet();
  AST val = MakeBool(false);
  AST type = type::MakeList("foo", true, type::MakeBool("Element", true));
  EXPECT_DEATH({ Insert(type, val, &set); }, ".*");
}

// Crashes if the set element has the wrong type.
TEST(ValueDeathTest, InsertRequiresTypedArgument) {
  AST set = MakeEmptySet();
  AST val = MakeInt(5);
  AST type = type::MakeSet("foo", true, type::MakeBool("Element", true));
  EXPECT_DEATH({ Insert(type, val, &set); }, ".*");
}

// Crashes if the set pointer is null.
TEST(ValueDeathTest, InsertRequiresNonNullPointer) {
  AST val = MakeInt(5);
  AST type = type::MakeSet("foo", true, type::MakeBool("Element", true));
  EXPECT_DEATH({ Insert(type, val, nullptr); }, ".*");
}

// Crashes if the pointer 'set' is to a non-set.
TEST(ValueDeathTest, InsertRequiresSet) {
  AST val = MakeInt(5);
  AST list = MakeEmptyList();
  AST type = type::MakeSet("foo", true, type::MakeBool("Element", true));
  EXPECT_DEATH({ Insert(type, val, &list); }, ".*");
}

// Test set creation and insertion.
TEST_F(ValueTest, ConstructsSets) {
  type_ = type::MakeSet("foo", true, type::MakeBool("Element", true));
  AST list_type = type::MakeList("foo", true, type::MakeBool("Element", true));
  val_ = MakeEmptySet();
  EXPECT_TRUE(IsValue(val_, &err_));
  EXPECT_TRUE(type::IsTyped(type_, val_, &err_));
  EXPECT_FALSE(type::IsTyped(list_type, val_, &err_));
  EXPECT_EQ(Size(val_), 0);
  // After inserting one element, the set has one element.
  Insert(type_, MakeBool(true), &val_);
  EXPECT_TRUE(IsValue(val_, &err_));
  EXPECT_TRUE(type::IsTyped(type_, val_, &err_));
  EXPECT_EQ(Size(val_), 1);
  // After inserting the same element twice, the set has one element.
  Insert(type_, MakeBool(true), &val_);
  EXPECT_EQ(Size(val_), 1);
  // After inserting two distinct elements, the set has two elements.
  Insert(type_, MakeBool(false), &val_);
  EXPECT_EQ(Size(val_), 2);
}

static AST GetTupleType() {
  vector<AST> field_asts;
  field_asts.emplace_back(type::MakeBool("First", true));
  field_asts.emplace_back(type::MakeString("Second", true));
  return type::MakeTuple("Tuple", true, field_asts);
}

// Test fatal input arguments requirements of SetField.
// Crashes if a value is passed as a type.
TEST(ValueDeathTest, SetFieldRequiresAType) {
  AST tuple = MakeNullTuple(2);
  AST val = MakeBool(false);
  EXPECT_DEATH({ SetField(tuple, 0, val, &tuple); }, ".*");
}

// Crashes if a non-tuple type is passed to SetField.
TEST(ValueDeathTest, SetFieldRequiresATupleType) {
  AST tuple = MakeNullTuple(2);
  AST val = MakeBool(false);
  EXPECT_DEATH({ SetField(tuple, 0, val, &tuple); }, ".*");
}

// Crashes if the field number is not valid.
TEST(ValueDeathTest, SetFieldRequiresValidFieldNumber) {
  AST tuple = MakeNullTuple(2);
  AST val = MakeBool(true);
  EXPECT_DEATH({ SetField(GetTupleType(), 2, val, &tuple); }, ".*");
}

// Crashes if the tuple pointer is null.
TEST(ValueDeathTest, SetFieldRequiresNonNullPointer) {
  AST val = MakeBool(true);
  EXPECT_DEATH({ SetField(GetTupleType(), 0, val, nullptr); }, ".*");
}

// Crashes if the pointer 'tuple' is to a non-tuple.
TEST(ValueDeathTest, SetFieldRequiresSet) {
  AST val = MakeBool(true);
  AST list = MakeEmptyList();
  EXPECT_DEATH({ SetField(GetTupleType(), 0, val, &list); }, ".*");
}

// Test tuple creation and instantiation.
TEST_F(ValueTest, ConstructsTuples) {
  type_ = GetTupleType();
  val_ = MakeNullTuple(2);
  EXPECT_TRUE(IsValue(val_, &err_));
  SetField(type_, 0, MakeBool(true), &val_);
  SetField(type_, 1, MakeString("foo"), &val_);
  EXPECT_TRUE(type::IsTyped(type_, val_, &err_));
  EXPECT_EQ(Size(val_), 2);
}

}  // namespace
}  // namespace value
}  // namespace ast
}  // namespace tervuren
