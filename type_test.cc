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

#include "third_party/logle/type.h"

#include <vector>

#include "third_party/logle/gtest.h"
#include "third_party/logle/type_checker.h"

namespace third_party_logle {
namespace ast {
namespace type {
namespace {

class TypeTest : public ::testing::Test {
 protected:
  AST type_ast_;
  AST arg_ast_;
  string err_;
};

// Test creation of the null type.
TEST_F(TypeTest, CreatesNullType) {
  type_ast_ = MakeNull("foo");
  EXPECT_TRUE(IsType(type_ast_, &err_));
}

// Test creation of primitive types.
TEST_F(TypeTest, CreatesABool) {
  type_ast_ = MakeBool("foo", true);
  EXPECT_TRUE(IsType(type_ast_, &err_));
}

TEST_F(TypeTest, CreatesAnInt) {
  type_ast_ = MakeInt("foo", false);
  EXPECT_TRUE(IsType(type_ast_, &err_));
}

TEST_F(TypeTest, CreatesAString) {
  type_ast_ = MakeString("foo", false);
  EXPECT_TRUE(IsType(type_ast_, &err_));
}

TEST_F(TypeTest, CreatesATimestamp) {
  type_ast_ = MakeTimestamp("foo", false);
  EXPECT_TRUE(IsType(type_ast_, &err_));
}

// Test creation of intervals.
TEST_F(TypeTest, CreatesAnInterval) {
  type_ast_ = MakeInterval("foo", PrimitiveType::TIMESTAMP);
  EXPECT_TRUE(IsType(type_ast_, &err_));
}

// Test creation of composite types. Create a type for a list of timestamps.
TEST_F(TypeTest, CreatesAList) {
  arg_ast_ = MakeTimestamp("foo", false);
  type_ast_ = MakeList("bar", false, arg_ast_);
  EXPECT_TRUE(IsType(type_ast_, &err_));
}

// Create a type for a set of timestamps.
TEST_F(TypeTest, CreatesASet) {
  arg_ast_ = MakeTimestamp("foo", false);
  type_ast_ = MakeSet("bar", false, arg_ast_);
  EXPECT_TRUE(IsType(type_ast_, &err_));
}

// Create a type for a tuple of a timestamp and a string.
TEST_F(TypeTest, CreatesATuple) {
  vector<AST> args;
  arg_ast_ = MakeTimestamp("foo", false);
  args.emplace_back(arg_ast_);
  arg_ast_ = MakeString("bar", false);
  args.emplace_back(arg_ast_);
  type_ast_ = MakeTuple("baz", false, args);
  EXPECT_TRUE(IsType(type_ast_, &err_));
}

// Creates types for directories and files.
TEST_F(TypeTest, FileTypes) {
  type_ast_ = MakeFile();
  // A file is a tuple consisting of a file path and a file name.
  // EXPECT_TRUE(type_ast_.has_c_ast());
  EXPECT_TRUE(type_ast_.c_ast().op() == Operator::TUPLE);
  EXPECT_TRUE(type_ast_.c_ast().arg_size() == 2);
  // The file path is a list of strings.
  AST path = type_ast_.c_ast().arg(0);
  EXPECT_TRUE(path.c_ast().op() == Operator::LIST);
  EXPECT_TRUE(path.c_ast().arg_size() == 1);
  EXPECT_TRUE(path.c_ast().arg(0).has_p_ast());
  EXPECT_TRUE(path.c_ast().arg(0).p_ast().type() == PrimitiveType::STRING);
  // The filename is a string.
  AST filename = type_ast_.c_ast().arg(1);
  EXPECT_TRUE(filename.p_ast().type() == PrimitiveType::STRING);
}

// Check that non-null but non-type arguments make composite methods crash.
TEST(TypeDeathTest, NonTypeASTToListArg) {
  AST arg, type;
  arg.clear_name();
  EXPECT_DEATH({ type = MakeList("foo", false, arg); }, ".*");
}

TEST(TypeDeathTest, NonTypeASTToSetArg) {
  AST arg, type;
  arg.clear_name();
  EXPECT_DEATH({ type = MakeSet("foo", false, arg); }, ".*");
}

TEST(TypeDeathTest, NonTypeASTToTupleArg) {
  AST arg, type;
  vector<AST> args;
  arg = MakeBool("foo", false);
  args.emplace_back(arg);
  arg.clear_is_nullable();
  args.emplace_back(arg);
  EXPECT_DEATH({ type = MakeTuple("bar", false, args); }, ".*");
}

TEST(TypeDeathTest, NonTypeASTToCompositeArg) {
  AST arg, type;
  vector<AST> args;
  arg = MakeBool("foo", false);
  args.emplace_back(arg);
  arg.clear_is_nullable();
  args.emplace_back(arg);
  EXPECT_DEATH({
    type = MakeComposite("bar", false, Operator::TUPLE, args); }, ".*");
}

}  // namespace
}  // namespace type
}  // namespace ast
}  // namespace third_party_logle
