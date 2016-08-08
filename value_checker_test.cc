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

#include <map>
#include "base/vector.h"

#include "ast.h"
#include "base/string.h"
#include "gtest.h"
#include "value_checker.h"

namespace tervuren {
namespace ast {
namespace value {
namespace {

static void Initialize(PrimitiveType type, AST* ast) {
  ast->clear_name();
  ast->clear_is_nullable();
  ast->mutable_p_ast()->set_type(type);
}

static void Initialize(Operator op, const AST& arg, AST* ast) {
  ast->clear_name();
  ast->clear_is_nullable();
  ast->mutable_c_ast()->set_op(op);
  AST* a = ast->mutable_c_ast()->add_arg();
  *a = arg;
}

static const map<string, bool> bool_vals = {
    {"", false}, {"true", true}, {"false", true}, {"0", false}, {"1", false} };

// The set of tests below check if ASTs represent values.
// Accept the representation of null values.
TEST(ValueCheckerTest, IsNullValue) {
  AST ast;
  string err;
  ast.clear_name();
  ast.clear_is_nullable();
  ast.set_allocated_p_ast(nullptr);
  EXPECT_TRUE(IsValue(ast, &err));
}

// Test representation of Boolean values.
TEST(ValueCheckerTest, IsABool) {
  AST ast;
  string err;
  PrimitiveType ptype = PrimitiveType::BOOL;
  Initialize(ptype, &ast);

  ast.mutable_p_ast()->mutable_val()->set_bool_val(true);
  EXPECT_TRUE(IsPrimitive(ptype, ast.p_ast().val()));
  EXPECT_TRUE(IsValue(ast, &err));

  ast.mutable_p_ast()->mutable_val()->set_bool_val(false);
  EXPECT_TRUE(IsPrimitive(ptype, ast.p_ast().val()));
  EXPECT_TRUE(IsValue(ast, &err));

  ast.mutable_p_ast()->mutable_val()->set_int_val(0);
  EXPECT_FALSE(IsPrimitive(ptype, ast.p_ast().val()));
  EXPECT_FALSE(IsValue(ast, &err));

  ast.mutable_p_ast()->mutable_val()->set_string_val("");
  EXPECT_FALSE(IsPrimitive(ptype, ast.p_ast().val()));
  EXPECT_FALSE(IsValue(ast, &err));

  ast.mutable_p_ast()->clear_val();
  EXPECT_TRUE(IsValue(ast, &err));
}

// Test representation of int values.
static const map<string, bool> int_vals = {
  {"", false}, {"0", true}, {"00", true}, {"-0", true}, {"-00", true},
  {"1", true}, {"01", true}, {"-1", true}, {"-01", true}, {"a", false},
  {"1a", false}, {"-1a", false} };

TEST(ValueCheckerTest, IsAnInt) {
  AST ast;
  string err;
  PrimitiveType ptype = PrimitiveType::INT;
  Initialize(ptype, &ast);

  ast.mutable_p_ast()->mutable_val()->set_int_val(0);
  EXPECT_TRUE(IsPrimitive(ptype, ast.p_ast().val()));
  EXPECT_TRUE(IsValue(ast, &err));

  ast.mutable_p_ast()->mutable_val()->set_int_val(-1);
  EXPECT_TRUE(IsPrimitive(ptype, ast.p_ast().val()));
  EXPECT_TRUE(IsValue(ast, &err));

  ast.mutable_p_ast()->mutable_val()->set_string_val("0");
  EXPECT_FALSE(IsPrimitive(ptype, ast.p_ast().val()));
  EXPECT_FALSE(IsValue(ast, &err));

  ast.mutable_p_ast()->mutable_val()->set_time_val(0);
  EXPECT_FALSE(IsPrimitive(ptype, ast.p_ast().val()));
  EXPECT_FALSE(IsValue(ast, &err));

  ast.mutable_p_ast()->clear_val();
  EXPECT_TRUE(IsValue(ast, &err));
}

TEST(ValueCheckerTest, IsAString) {
  AST ast;
  string err;
  PrimitiveType ptype = PrimitiveType::STRING;
  Initialize(ptype, &ast);

  ast.mutable_p_ast()->mutable_val()->set_bool_val(true);
  EXPECT_FALSE(IsPrimitive(ptype, ast.p_ast().val()));
  EXPECT_FALSE(IsValue(ast, &err));

  ast.mutable_p_ast()->mutable_val()->set_int_val(0);
  EXPECT_FALSE(IsPrimitive(ptype, ast.p_ast().val()));
  EXPECT_FALSE(IsValue(ast, &err));

  ast.mutable_p_ast()->mutable_val()->set_string_val("a");
  EXPECT_TRUE(IsPrimitive(ptype, ast.p_ast().val()));
  EXPECT_TRUE(IsValue(ast, &err));

  ast.mutable_p_ast()->clear_val();
  EXPECT_TRUE(IsValue(ast, &err));
}

TEST(ValueCheckerTest, IsATimestamp) {
  AST ast;
  string err;
  PrimitiveType ptype = PrimitiveType::TIMESTAMP;

  Initialize(ptype, &ast);
  ast.mutable_p_ast()->mutable_val()->set_time_val(-5);
  EXPECT_TRUE(IsPrimitive(ptype, ast.p_ast().val()));
  EXPECT_TRUE(IsValue(ast, &err));

  ast.mutable_p_ast()->clear_val();
  EXPECT_TRUE(IsValue(ast, &err));
}

TEST(ValueCheckerTest, IsAnInterval) {
  AST zero, one, btrue, ast;
  Initialize(PrimitiveType::INT, &zero);
  zero.mutable_p_ast()->mutable_val()->set_int_val(0);
  Initialize(PrimitiveType::INT, &one);
  zero.mutable_p_ast()->mutable_val()->set_int_val(1);
  Initialize(PrimitiveType::BOOL, &btrue);
  btrue.mutable_p_ast()->mutable_val()->set_bool_val(false);

  string err;
  // An interval cannot have only one argument.
  Initialize(Operator::INTERVAL, zero, &ast);
  EXPECT_FALSE(IsValue(ast, &err));

  // An interval with two arguments is a value.
  AST* arg = ast.mutable_c_ast()->add_arg();
  *arg = one;
  EXPECT_TRUE(IsValue(ast, &err));

  // An interval with two arguments one of which is null, is a value.
  ast.mutable_c_ast()->mutable_arg(0)->mutable_p_ast()->clear_val();
  EXPECT_TRUE(IsValue(ast, &err));

  *(ast.mutable_c_ast()->mutable_arg(0)) = zero;
  ast.mutable_c_ast()->mutable_arg(1)->mutable_p_ast()->clear_val();
  EXPECT_TRUE(IsValue(ast, &err));

  // An interval with two arguments of different type is not a value.
  *(ast.mutable_c_ast()->mutable_arg(0)) = btrue;
  EXPECT_FALSE(IsValue(ast, &err));

  // An interval with an argument that is not a value is not a value, even if
  // that argument has the appropriate type.
  arg = ast.mutable_c_ast()->mutable_arg(0);
  arg->mutable_p_ast()->set_type(PrimitiveType::INT);
  EXPECT_FALSE(IsValue(ast, &err));

  // An interval with two null arguments is a value.
  ast.mutable_c_ast()->mutable_arg(0)->mutable_p_ast()->clear_val();
  ast.mutable_c_ast()->mutable_arg(1)->mutable_p_ast()->clear_val();
  EXPECT_TRUE(IsValue(ast, &err));

  // An interval with no arguments is a value.
  ast.mutable_c_ast()->clear_arg();
  EXPECT_TRUE(IsValue(ast, &err));
}

static void TestContainer(Operator op) {
  AST zero, ast;
  Initialize(PrimitiveType::INT, &zero);
  zero.mutable_p_ast()->mutable_val()->set_int_val(0);

  string err;
  // A container with an int is a value.
  Initialize(op, zero, &ast);
  EXPECT_TRUE(IsValue(ast, &err));

  // A container with an AST that is not a value is not a value.
  AST* arg = ast.mutable_c_ast()->mutable_arg(0);
  arg->mutable_p_ast()->mutable_val()->set_string_val("a");
  EXPECT_FALSE(IsValue(ast, &err));

  // Due to imprecision in value checking, a container with one int and one bool
  // is considered a value.
  arg->mutable_p_ast()->mutable_val()->set_int_val(1);
  arg = ast.mutable_c_ast()->add_arg();
  Initialize(PrimitiveType::BOOL, arg);
  arg->mutable_p_ast()->mutable_val()->set_bool_val(false);
  EXPECT_TRUE(IsValue(ast, &err));

  // The empty container is a value.
  ast.mutable_c_ast()->clear_arg();
  EXPECT_TRUE(IsValue(ast, &err));
}

TEST(ValueCheckerTest, ListIsLikeAContainer) {
  TestContainer(Operator::LIST);
}

TEST(ValueCheckerTest, SetIsLikeAContainer) {
  TestContainer(Operator::SET);
}

TEST(ValueCheckerTest, TupleIsLikeAContainer) {
  TestContainer(Operator::TUPLE);
}

// Tests of isomorphism checks.
// Null values with no additional type information are isomorphic.
TEST(IsomorphismTest, NullIsomorphism) {
  AST val1, val2;
  EXPECT_TRUE(Isomorphic(val1, val2));
  val1.mutable_p_ast()->set_type(PrimitiveType::BOOL);
  EXPECT_FALSE(Isomorphic(val1, val2));
  val1.mutable_c_ast()->set_op(Operator::SET);
  EXPECT_FALSE(Isomorphic(val1, val2));
}

// Null values of the same type are isomorphic.
TEST(IsomorphismTest, PrimitiveNullIsomorphism) {
  AST val1, val2;
  val1.mutable_p_ast()->set_type(PrimitiveType::BOOL);
  val2.mutable_p_ast()->set_type(PrimitiveType::BOOL);
  val1.mutable_p_ast()->clear_val();
  val2.mutable_p_ast()->mutable_val()->set_bool_val(true);
  EXPECT_FALSE(Isomorphic(val1, val2));

  val2.mutable_p_ast()->clear_val();
  EXPECT_TRUE(Isomorphic(val1, val2));
  // Two null values must have the same type to be isomorphic.
  val2.mutable_p_ast()->set_type(PrimitiveType::INT);
  EXPECT_FALSE(Isomorphic(val1, val2));
}

TEST(IsomorphismTest, BoolIsomorphism) {
  AST val1, val2;
  Initialize(PrimitiveType::BOOL, &val1);
  val1.mutable_p_ast()->mutable_val()->set_bool_val(false);
  Initialize(PrimitiveType::BOOL, &val2);
  val2.mutable_p_ast()->mutable_val()->set_bool_val(false);
  EXPECT_TRUE(Isomorphic(val1, val2));

  // ASTs with different values are not isomoprhic.
  val1.mutable_p_ast()->mutable_val()->set_bool_val(true);
  EXPECT_FALSE(Isomorphic(val1, val2));

  // ASTs with different types but same value are not isomoprhic.
  val2.mutable_p_ast()->mutable_val()->set_bool_val(true);
  val2.mutable_p_ast()->set_type(PrimitiveType::INT);
  EXPECT_FALSE(Isomorphic(val1, val2));
}

TEST(IsomorphismTest, IntIsomorphism) {
  AST val1, val2;
  Initialize(PrimitiveType::INT, &val1);
  val1.mutable_p_ast()->mutable_val()->set_int_val(0);
  Initialize(PrimitiveType::INT, &val2);
  val2.mutable_p_ast()->mutable_val()->set_int_val(0);
  EXPECT_TRUE(Isomorphic(val1, val2));

  val2.mutable_p_ast()->mutable_val()->set_int_val(1);
  EXPECT_FALSE(Isomorphic(val1, val2));

  // Not isomorphic because values are same but types are different.
  val2.mutable_p_ast()->mutable_val()->set_int_val(0);
  val2.mutable_p_ast()->set_type(PrimitiveType::BOOL);
  EXPECT_FALSE(Isomorphic(val1, val2));
}

TEST(IsomorphismTest, StringIsomorphism) {
  AST val1, val2;
  Initialize(PrimitiveType::STRING, &val1);
  val1.mutable_p_ast()->mutable_val()->set_string_val("");
  Initialize(PrimitiveType::STRING, &val2);
  val2.mutable_p_ast()->mutable_val()->set_string_val("");
  EXPECT_TRUE(Isomorphic(val1, val2));

  val2.mutable_p_ast()->mutable_val()->set_string_val("a");
  EXPECT_FALSE(Isomorphic(val1, val2));
}

TEST(IsomorphismTest, IntervalIsomorphism) {
  AST val1, val2, zero, one, two;
  Initialize(PrimitiveType::INT, &zero);
  zero.mutable_p_ast()->mutable_val()->set_int_val(0);
  Initialize(PrimitiveType::INT, &one);
  one.mutable_p_ast()->mutable_val()->set_int_val(1);
  Initialize(PrimitiveType::INT, &two);
  two.mutable_p_ast()->mutable_val()->set_int_val(2);

  Initialize(Operator::INTERVAL, zero, &val1);
  Initialize(Operator::INTERVAL, zero, &val2);
  AST* ub = val1.mutable_c_ast()->add_arg();
  *ub = one;
  ub = val2.mutable_c_ast()->add_arg();
  *ub = one;
  EXPECT_TRUE(Isomorphic(val1, val2));

  // Intervals over different ranges are not isomorphic.
  *(val2.mutable_c_ast()->mutable_arg(1)) = two;
  EXPECT_FALSE(Isomorphic(val1, val2));

  // Empty intervals have multiple non-isomorphic representations.
  *(val1.mutable_c_ast()->mutable_arg(0)) = one;
  *(val1.mutable_c_ast()->mutable_arg(1)) = zero;
  *(val2.mutable_c_ast()->mutable_arg(0)) = two;
  *(val2.mutable_c_ast()->mutable_arg(1)) = zero;
  EXPECT_FALSE(Isomorphic(val1, val2));
}

void CheckIsomorphism(Operator op, AST* val1, AST* val2) {
  AST zero;
  AST one;
  AST two;
  Initialize(PrimitiveType::INT, &zero);
  zero.mutable_p_ast()->mutable_val()->set_int_val(0);
  Initialize(PrimitiveType::INT, &one);
  one.mutable_p_ast()->mutable_val()->set_int_val(1);
  Initialize(PrimitiveType::INT, &two);
  two.mutable_p_ast()->mutable_val()->set_int_val(2);

  Initialize(op, zero, val1);
  EXPECT_FALSE(Isomorphic(*val1, *val2));
  Initialize(op, zero, val2);
  EXPECT_TRUE(Isomorphic(*val1, *val2));

  // Isomorphism fails because val1 has two arguments but val2 has one.
  AST* second = val1->mutable_c_ast()->add_arg();
  *second = one;
  EXPECT_FALSE(Isomorphic(*val1, *val2));

  // Isomorphism succeeds because both values have the same number of arguments.
  second = val2->mutable_c_ast()->add_arg();
  *second = one;
  EXPECT_TRUE(Isomorphic(*val1, *val2));

  // Both values have two arguments but the second arguments are different.
  *(val2->mutable_c_ast()->mutable_arg(1)) = two;
  EXPECT_FALSE(Isomorphic(*val1, *val2));

  val1->mutable_c_ast()->clear_arg();
  EXPECT_FALSE(Isomorphic(*val1, *val2));
  val2->mutable_c_ast()->clear_arg();
  EXPECT_TRUE(Isomorphic(*val1, *val2));
}


TEST(IsomorphismTest, ListIsomorphism) {
  AST val1, val2;
  CheckIsomorphism(Operator::LIST, &val1, &val2);
}

TEST(IsomorphismTest, SetIsomorphism) {
  AST val1, val2;
  CheckIsomorphism(Operator::SET, &val1, &val2);

  // The set {1, 2} has two non-isomoprhic representations depending on the
  // order in which elements are added.
  AST one;
  AST two;
  Initialize(PrimitiveType::INT, &one);
  one.mutable_p_ast()->mutable_val()->set_int_val(1);
  Initialize(PrimitiveType::INT, &two);
  two.mutable_p_ast()->mutable_val()->set_int_val(2);

  AST* arg = val1.mutable_c_ast()->add_arg();
  *arg = one;
  arg = val1.mutable_c_ast()->add_arg();
  *arg = two;
  EXPECT_FALSE(Isomorphic(val1, val2));

  arg = val2.mutable_c_ast()->add_arg();
  *arg = two;
  arg = val2.mutable_c_ast()->add_arg();
  *arg = one;
  EXPECT_FALSE(Isomorphic(val1, val2));
}

TEST(IsomorphismTest, TupleIsomorphism) {
  AST val1, val2;
  CheckIsomorphism(Operator::TUPLE, &val1, &val2);

  // The tuple (1, 2) has two non-isomoprhic representations depending on the
  // order in which elements are added.
  AST one;
  AST two;
  Initialize(PrimitiveType::INT, &one);
  one.mutable_p_ast()->mutable_val()->set_int_val(1);
  Initialize(PrimitiveType::INT, &two);
  two.mutable_p_ast()->mutable_val()->set_int_val(2);

  AST* arg = val1.mutable_c_ast()->add_arg();
  *arg = one;
  arg = val1.mutable_c_ast()->add_arg();
  *arg = two;
  EXPECT_FALSE(Isomorphic(val1, val2));

  arg = val2.mutable_c_ast()->add_arg();
  *arg = two;
  arg = val2.mutable_c_ast()->add_arg();
  *arg = one;
  EXPECT_FALSE(Isomorphic(val1, val2));
}

// Test canonicalization methods.
//
// Null values of the same type are unaffected by canonicalization.
TEST(CanonicalizerTest, NullCanonicalization) {
  AST val1, val2;
  val1.mutable_p_ast()->set_type(PrimitiveType::BOOL);
  val2.mutable_p_ast()->set_type(PrimitiveType::BOOL);
  val1.mutable_p_ast()->clear_val();
  val2.mutable_p_ast()->clear_val();
  Canonicalize(&val2);
  EXPECT_TRUE(Isomorphic(val1, val2));
}

void MakeBoolInterval(bool lb, bool ub, AST* val) {
  AST lbval, ubval;
  Initialize(PrimitiveType::BOOL, &lbval);
  lbval.mutable_p_ast()->mutable_val()->set_bool_val(lb);
  Initialize(PrimitiveType::BOOL, &ubval);
  ubval.mutable_p_ast()->mutable_val()->set_bool_val(ub);

  val->clear_name();
  val->clear_is_nullable();
  val->mutable_c_ast()->set_op(Operator::INTERVAL);
  AST* arg = val->mutable_c_ast()->add_arg();
  *arg = lbval;
  arg = val->mutable_c_ast()->add_arg();
  *arg = ubval;
}

void CanonicalizeBoolIntervals(bool lb1, bool ub1, bool lb2, bool ub2,
                               bool before, bool after) {
  AST itv1, itv2;
  MakeBoolInterval(lb1, ub1, &itv1);
  MakeBoolInterval(lb2, ub2, &itv2);
  EXPECT_EQ(Isomorphic(itv1, itv2), before);
  Canonicalize(&itv1);
  Canonicalize(&itv2);
  EXPECT_EQ(Isomorphic(itv1, itv2), after);
}

TEST(CanonicalizerTest, BoolIntervalCanonicalization) {
  // Isomorphic before and after canonicalization.
  CanonicalizeBoolIntervals(true, true, true, true, true, true);
  CanonicalizeBoolIntervals(true, false, true, false, true, true);
  // Not isomorphic before or after canonicalization.
  CanonicalizeBoolIntervals(true, false, false, true, false, false);
  // The interval [true, false] is isomorphic to the empty interval after
  // canonicalization.
  AST itv1, itv2;
  MakeBoolInterval(true, false, &itv1);
  itv1.mutable_c_ast()->clear_arg();
  MakeBoolInterval(true, false, &itv2);
  EXPECT_FALSE(Isomorphic(itv1, itv2));
  Canonicalize(&itv1);
  Canonicalize(&itv2);
  EXPECT_TRUE(Isomorphic(itv1, itv2));
}

void MakeIntInterval(int lb, int ub, AST* val) {
  AST lbval, ubval;
  Initialize(PrimitiveType::INT, &lbval);
  lbval.mutable_p_ast()->mutable_val()->set_int_val(lb);
  Initialize(PrimitiveType::INT, &ubval);
  ubval.mutable_p_ast()->mutable_val()->set_int_val(ub);

  val->clear_name();
  val->clear_is_nullable();
  val->mutable_c_ast()->set_op(Operator::INTERVAL);
  AST* arg = val->mutable_c_ast()->add_arg();
  *arg = lbval;
  arg = val->mutable_c_ast()->add_arg();
  *arg = ubval;
}

void CanonicalizeIntIntervals(int lb1, int ub1, int lb2, int ub2, int before,
                              int after) {
  AST itv1, itv2;
  MakeIntInterval(lb1, ub1, &itv1);
  MakeIntInterval(lb2, ub2, &itv2);
  EXPECT_EQ(Isomorphic(itv1, itv2), before);
  Canonicalize(&itv1);
  Canonicalize(&itv2);
  EXPECT_EQ(Isomorphic(itv1, itv2), after);
}

TEST(CanonicalizerTest, IntIntervalCanonicalization) {
  // Isomorphic before and after canonicalization.
  CanonicalizeIntIntervals(0, 1, 0, 1, true, true);
  // Not isomorphic before or after canonicalization.
  CanonicalizeIntIntervals(0, -1, -1, 0, false, false);
  // Isomophic to the empty interval, after canonicalization.
  CanonicalizeIntIntervals(0, -1, 3, 1, false, true);
}

void MakeIntContainer(Operator op, const vector<int>& args, AST* val) {
  val->clear_name();
  val->clear_is_nullable();
  val->mutable_c_ast()->set_op(op);
  val->mutable_c_ast()->clear_arg();
  AST element;
  AST* arg_ptr;
  for (int arg : args) {
    Initialize(PrimitiveType::INT, &element);
    element.mutable_p_ast()->mutable_val()->set_int_val(arg);
    arg_ptr = val->mutable_c_ast()->add_arg();
    *arg_ptr = element;
  }
}

void MakeCompositeContainer(Operator op, const vector<AST>& args, AST* val) {
  val->clear_name();
  val->clear_is_nullable();
  val->mutable_c_ast()->set_op(op);
  val->mutable_c_ast()->clear_arg();
  AST* arg_ptr;
  for (const auto& arg : args) {
    arg_ptr = val->mutable_c_ast()->add_arg();
    *arg_ptr = arg;
  }
}

TEST(CanonicalizerTest, IntervalListCanonicalization) {
  AST itv1;
  MakeBoolInterval(true, false, &itv1);
  vector<AST> args;

  // A list containing the empty interval and one containing the interval
  // [true,false] are only isomorphic after canonicalization.
  AST list1, list2;
  args.push_back(itv1);
  MakeCompositeContainer(Operator::LIST, args, &list1);
  args[0].mutable_c_ast()->clear_arg();
  MakeCompositeContainer(Operator::LIST, args, &list2);
  EXPECT_FALSE(Isomorphic(list1, list2));

  Canonicalize(&list1);
  Canonicalize(&list2);
  EXPECT_TRUE(Isomorphic(list1, list2));
}

TEST(CanonicalizerTest, SetCanonicalization) {
  vector<int> args;
  AST set1, set2;
  // The set { 0 } has non-isomorphic representations, with 0 occurring multiple
  // times.
  args.emplace_back(0);
  MakeIntContainer(Operator::SET, args, &set1);
  args.emplace_back(0);
  MakeIntContainer(Operator::SET, args, &set2);
  EXPECT_FALSE(Isomorphic(set1, set2));
  Canonicalize(&set2);
  EXPECT_TRUE(Isomorphic(set1, set2));
  // The set { 0, 1 } has two, non-isomorphic representations.
  args[1] = 1;
  MakeIntContainer(Operator::SET, args, &set1);
  args[0] = 1;
  args[1] = 0;
  MakeIntContainer(Operator::SET, args, &set2);
  EXPECT_FALSE(Isomorphic(set1, set2));
  Canonicalize(&set2);
  EXPECT_TRUE(Isomorphic(set1, set2));
}

TEST(CanonicalizerTest, TupleCanonicalization) {
  AST itv1;
  AST one;
  MakeBoolInterval(true, false, &itv1);
  Initialize(PrimitiveType::INT, &one);
  one.mutable_p_ast()->mutable_val()->set_int_val(1);

  vector<AST> args;

  // A tuple containing the empty interval and one containing the interval
  // [true,false] are only isomorphic after canonicalization.
  AST tuple1, tuple2;
  args.push_back(itv1);
  args.push_back(one);
  MakeCompositeContainer(Operator::LIST, args, &tuple1);
  args[0].mutable_c_ast()->clear_arg();
  MakeCompositeContainer(Operator::LIST, args, &tuple2);
  EXPECT_FALSE(Isomorphic(tuple1, tuple2));

  Canonicalize(&tuple1);
  Canonicalize(&tuple2);
  EXPECT_TRUE(Isomorphic(tuple1, tuple2));
}

}  // namespace
}  // namespace value
}  // namespace ast
}  // namespace tervuren
