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
#include "ast.h"

#include "base/string.h"
#include "gtest.h"

namespace morphie {
namespace ast {
namespace {

class ASTTest : public ::testing::Test {
 protected:
  AST ast_;
  TaggedAST tast_;
};

TEST_F(ASTTest, IsNullAST) {
  EXPECT_TRUE(IsNull(ast_));
  ast_.mutable_p_ast()->set_type(PrimitiveType::BOOL);
  EXPECT_FALSE(IsNull(ast_));
}

TEST_F(ASTTest, IsAPrimitiveAST) {
  ast_.mutable_p_ast()->set_type(PrimitiveType::BOOL);
  EXPECT_TRUE(IsBool(ast_));
  ast_.mutable_p_ast()->set_type(PrimitiveType::INT);
  EXPECT_TRUE(IsInt(ast_));
  ast_.mutable_p_ast()->set_type(PrimitiveType::STRING);
  EXPECT_TRUE(IsString(ast_));
  ast_.mutable_p_ast()->set_type(PrimitiveType::TIMESTAMP);
  EXPECT_TRUE(IsTimestamp(ast_));
}

// A container is a list, tuple or set. An interval is the only composite type
// that is not a container.
TEST_F(ASTTest, IsAContainerAST) {
  ast_.mutable_c_ast()->set_op(Operator::INTERVAL);
  EXPECT_FALSE(IsContainer(ast_));
  ast_.mutable_c_ast()->set_op(Operator::LIST);
  EXPECT_TRUE(IsContainer(ast_));
  EXPECT_TRUE(IsList(ast_));
  ast_.mutable_c_ast()->set_op(Operator::TUPLE);
  EXPECT_TRUE(IsContainer(ast_));
  EXPECT_TRUE(IsTuple(ast_));
  ast_.mutable_c_ast()->set_op(Operator::SET);
  EXPECT_TRUE(IsContainer(ast_));
  EXPECT_TRUE(IsSet(ast_));
}

// Test that a name field is serialized as the empty string if absent and that
// if no fields in the AST are set, then the type and the value are both 'null'.
TEST_F(ASTTest, PrintNull) {
  EXPECT_EQ("", ToString(ast_, PrintConfig(PrintOption::kName)));
  EXPECT_EQ("null", ToString(ast_, PrintConfig(PrintOption::kType)));
  EXPECT_EQ("null", ToString(ast_, PrintConfig(PrintOption::kValue)));
  EXPECT_EQ("null", ToString(ast_, PrintConfig(PrintOption::kNameAndType)));
  EXPECT_EQ("null : null",
            ToString(ast_, PrintConfig(PrintOption::kTypeAndValue)));
  EXPECT_EQ("null", ToString(ast_, PrintConfig(PrintOption::kNameAndValue)));
  EXPECT_EQ("null : null", ToString(ast_, PrintConfig(PrintOption::kAll)));
}

// Test every combination of printing the name, type and value information in an
// AST. These combinations are only tested for the Bool case.
TEST_F(ASTTest, PrintBool) {
  ast_.mutable_p_ast()->set_type(PrimitiveType::BOOL);
  ast_.set_is_nullable(false);
  ast_.set_name("b");
  EXPECT_EQ("b", ToString(ast_, PrintConfig(PrintOption::kName)));
  EXPECT_EQ("bool", ToString(ast_, PrintConfig(PrintOption::kType)));
  EXPECT_EQ("null", ToString(ast_, PrintConfig(PrintOption::kValue)));
  EXPECT_EQ("b bool", ToString(ast_, PrintConfig(PrintOption::kNameAndType)));
  EXPECT_EQ("bool : null",
            ToString(ast_, PrintConfig(PrintOption::kTypeAndValue)));
  EXPECT_EQ("b null", ToString(ast_, PrintConfig(PrintOption::kNameAndValue)));
  EXPECT_EQ("b bool : null", ToString(ast_, PrintConfig(PrintOption::kAll)));
  // A nullable bool type should serialize as "bool?".
  ast_.set_is_nullable(true);
  ast_.mutable_p_ast()->mutable_val()->set_bool_val(true);
  EXPECT_EQ("b bool? : true", ToString(ast_, PrintConfig(PrintOption::kAll)));
  ast_.mutable_p_ast()->mutable_val()->set_bool_val(false);
  EXPECT_EQ("b bool? : false", ToString(ast_, PrintConfig(PrintOption::kAll)));
}

TEST_F(ASTTest, PrintInt) {
  ast_.mutable_p_ast()->set_type(PrimitiveType::INT);
  ast_.set_is_nullable(true);
  ast_.set_name("i");
  EXPECT_EQ("i int? : null", ToString(ast_, PrintConfig(PrintOption::kAll)));
  ast_.mutable_p_ast()->mutable_val()->set_int_val(5);
  EXPECT_EQ("i int? : 5", ToString(ast_, PrintConfig(PrintOption::kAll)));
}

TEST_F(ASTTest, PrintString) {
  ast_.mutable_p_ast()->set_type(PrimitiveType::STRING);
  ast_.set_is_nullable(true);
  ast_.set_name("s");
  EXPECT_EQ("s string? : null", ToString(ast_, PrintConfig(PrintOption::kAll)));
  ast_.mutable_p_ast()->mutable_val()->set_string_val("foo");
  EXPECT_EQ("s string? : foo", ToString(ast_, PrintConfig(PrintOption::kAll)));
}

TEST_F(ASTTest, PrintTimestamp) {
  ast_.mutable_p_ast()->set_type(PrimitiveType::TIMESTAMP);
  ast_.set_is_nullable(true);
  ast_.set_name("t");
  EXPECT_EQ("t timestamp? : null",
            ToString(ast_, PrintConfig(PrintOption::kAll)));
  // 1 million microseconds after the start of the Unix epoch.
  ast_.mutable_p_ast()->mutable_val()->set_time_val(1000000);
  EXPECT_EQ("t timestamp? : 1970-01-01T00:00:01+00:00",
            ToString(ast_, PrintConfig(PrintOption::kAll)));
}

TEST_F(ASTTest, PrintInterval) {
  ast_.mutable_c_ast()->set_op(Operator::INTERVAL);
  ast_.set_is_nullable(true);
  ast_.set_name("i");
  EXPECT_EQ("i", ToString(ast_, PrintConfig(PrintOption::kName)));
  EXPECT_EQ("interval?(null)", ToString(ast_, PrintConfig(PrintOption::kType)));
  EXPECT_EQ("(null)", ToString(ast_, PrintConfig(PrintOption::kValue)));
  EXPECT_EQ("i interval?(null)",
            ToString(ast_, PrintConfig(PrintOption::kNameAndType)));
  EXPECT_EQ("i (null)",
            ToString(ast_, PrintConfig(PrintOption::kNameAndValue)));
  EXPECT_EQ("interval?(null)",
            ToString(ast_, PrintConfig(PrintOption::kTypeAndValue)));
  EXPECT_EQ("i interval?(null)",
            ToString(ast_, PrintConfig(PrintOption::kAll)));
  AST lb_ast, ub_ast;  // Lower and upper bounds.
  lb_ast.mutable_p_ast()->set_type(PrimitiveType::INT);
  lb_ast.mutable_p_ast()->mutable_val()->set_int_val(0);
  ub_ast.mutable_p_ast()->set_type(PrimitiveType::INT);
  ub_ast.mutable_p_ast()->mutable_val()->set_int_val(1);
  *(ast_.mutable_c_ast()->add_arg()) = lb_ast;
  *(ast_.mutable_c_ast()->add_arg()) = ub_ast;
  EXPECT_EQ("interval?(int, int)",
            ToString(ast_, PrintConfig(PrintOption::kType)));
  EXPECT_EQ("(0, 1)", ToString(ast_, PrintConfig(PrintOption::kValue)));
  EXPECT_EQ("i interval?(int, int)",
            ToString(ast_, PrintConfig(PrintOption::kNameAndType)));
  EXPECT_EQ("i (0, 1)",
            ToString(ast_, PrintConfig(PrintOption::kNameAndValue)));
  EXPECT_EQ("interval?(int : 0, int : 1)",
            ToString(ast_, PrintConfig(PrintOption::kTypeAndValue)));
  EXPECT_EQ("i interval?(int : 0, int : 1)",
            ToString(ast_, PrintConfig(PrintOption::kAll)));
  // Change the print configuration.
  PrintConfig config("[", "]", ":", PrintOption::kType);
  EXPECT_EQ("interval?[int:int]", ToString(ast_, config));
  config.set_opt(PrintOption::kValue);
  EXPECT_EQ("[0:1]", ToString(ast_, config));
}

TEST_F(ASTTest, PrintList) {
  ast_.mutable_c_ast()->set_op(Operator::LIST);
  ast_.set_is_nullable(true);
  ast_.set_name("l");
  AST arg;
  arg.mutable_p_ast()->set_type(PrimitiveType::INT);
  arg.mutable_p_ast()->mutable_val()->set_int_val(0);
  *(ast_.mutable_c_ast()->add_arg()) = arg;
  arg.mutable_p_ast()->set_type(PrimitiveType::INT);
  arg.mutable_p_ast()->mutable_val()->set_int_val(1);
  *(ast_.mutable_c_ast()->add_arg()) = arg;
  arg.mutable_p_ast()->set_type(PrimitiveType::INT);
  arg.mutable_p_ast()->mutable_val()->set_int_val(2);
  *(ast_.mutable_c_ast()->add_arg()) = arg;
  EXPECT_EQ("list?(int, int, int)", ToString(ast_, PrintOption::kType));
  EXPECT_EQ("(0, 1, 2)", ToString(ast_, PrintOption::kValue));
  EXPECT_EQ("l list?(int, int, int)",
            ToString(ast_, PrintOption::kNameAndType));
  EXPECT_EQ("l (0, 1, 2)", ToString(ast_, PrintOption::kNameAndValue));
  EXPECT_EQ("list?(int : 0, int : 1, int : 2)",
            ToString(ast_, PrintOption::kTypeAndValue));
  EXPECT_EQ("l list?(int : 0, int : 1, int : 2)",
            ToString(ast_, PrintOption::kAll));
  // Change print options to have a slash separated list.
  PrintConfig config("/", "", "/", PrintOption::kValue);
  EXPECT_EQ("/0/1/2", ToString(ast_, config));
  config.set_open("");
  EXPECT_EQ("0/1/2", ToString(ast_, config));
  config.set_close("/");
  EXPECT_EQ("0/1/2/", ToString(ast_, config));
}

TEST_F(ASTTest, PrintTuple) {
  ast_.mutable_c_ast()->set_op(Operator::TUPLE);
  ast_.set_is_nullable(true);
  ast_.set_name("t");
  AST int_arg, string_arg;
  int_arg.mutable_p_ast()->set_type(PrimitiveType::INT);
  int_arg.mutable_p_ast()->mutable_val()->set_int_val(0);
  *(ast_.mutable_c_ast()->add_arg()) = int_arg;
  string_arg.mutable_p_ast()->set_type(PrimitiveType::STRING);
  string_arg.mutable_p_ast()->mutable_val()->set_string_val("foo");
  *(ast_.mutable_c_ast()->add_arg()) = string_arg;
  EXPECT_EQ("tuple?(int, string)",
            ToString(ast_, PrintConfig(PrintOption::kType)));
  EXPECT_EQ("(0, foo)", ToString(ast_, PrintConfig(PrintOption::kValue)));
  EXPECT_EQ("t tuple?(int, string)",
            ToString(ast_, PrintConfig(PrintOption::kNameAndType)));
  EXPECT_EQ("t (0, foo)",
            ToString(ast_, PrintConfig(PrintOption::kNameAndValue)));
  EXPECT_EQ("tuple?(int : 0, string : foo)",
            ToString(ast_, PrintConfig(PrintOption::kTypeAndValue)));
  EXPECT_EQ("t tuple?(int : 0, string : foo)",
            ToString(ast_, PrintConfig(PrintOption::kAll)));
  PrintConfig config("", "", "\n", PrintOption::kValue);
  EXPECT_EQ("0\nfoo", ToString(ast_, config));
}

TEST_F(ASTTest, PrintSet) {
  ast_.mutable_c_ast()->set_op(Operator::SET);
  ast_.set_is_nullable(true);
  ast_.set_name("t");

  AST arg;
  arg.mutable_p_ast()->set_type(PrimitiveType::STRING);
  arg.mutable_p_ast()->mutable_val()->set_string_val("foo");
  *(ast_.mutable_c_ast()->add_arg()) = arg;
  arg.mutable_p_ast()->set_type(PrimitiveType::STRING);
  arg.mutable_p_ast()->mutable_val()->set_string_val("bar");
  *(ast_.mutable_c_ast()->add_arg()) = arg;
  arg.mutable_p_ast()->set_type(PrimitiveType::STRING);
  arg.mutable_p_ast()->mutable_val()->set_string_val("baz");
  *(ast_.mutable_c_ast()->add_arg()) = arg;

  EXPECT_EQ("set?(string, string, string)",
            ToString(ast_, PrintConfig(PrintOption::kType)));
  EXPECT_EQ("(foo, bar, baz)",
            ToString(ast_, PrintConfig(PrintOption::kValue)));
  EXPECT_EQ("t set?(string, string, string)",
            ToString(ast_, PrintConfig(PrintOption::kNameAndType)));
  EXPECT_EQ("t (foo, bar, baz)",
            ToString(ast_, PrintConfig(PrintOption::kNameAndValue)));
  EXPECT_EQ("set?(string : foo, string : bar, string : baz)",
            ToString(ast_, PrintConfig(PrintOption::kTypeAndValue)));
  EXPECT_EQ("t set?(string : foo, string : bar, string : baz)",
            ToString(ast_, PrintConfig(PrintOption::kAll)));
  PrintConfig config("", "", "\n", PrintOption::kValue);
  EXPECT_EQ("foo\nbar\nbaz", ToString(ast_, config));
}

}  // namespace
}  // namespace ast
}  // namespace morphie
