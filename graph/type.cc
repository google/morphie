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

#include "graph/type.h"

#include <cstdlib>

#include "graph/ast.h"
#include "graph/type_checker.h"
#include "util/logging.h"

namespace morphie {
namespace ast {
namespace type {

namespace {

const char kIntervalArg[] = "bound";
// Arguments of lists and sets do not have a name. List elements will be
// referred to by their position in the list. Set elements will be referred to
// their position in an enumeration of the set contents. This enumeration is not
// guaranteed to be unique.
const char kContainerArg[] = "";

void SetFields(const string& name, bool is_nullable, AST* ast) {
  ast->set_name(name);
  ast->set_is_nullable(is_nullable);
}

}  // namespace

// Neither of the oneof options is chosen in a null type.
AST MakeNull(const string& name) {
  AST ast;
  SetFields(name, true, &ast);
  return ast;
}

AST MakeBool(const string& name, bool is_nullable) {
  return MakePrimitive(name, is_nullable, PrimitiveType::BOOL);
}

AST MakeInt(const string& name, bool is_nullable) {
  return MakePrimitive(name, is_nullable, PrimitiveType::INT);
}

AST MakeString(const string& name, bool is_nullable) {
  return MakePrimitive(name, is_nullable, PrimitiveType::STRING);
}

AST MakeTimestamp(const string& name, bool is_nullable) {
  return MakePrimitive(name, is_nullable, PrimitiveType::TIMESTAMP);
}

AST MakePrimitive(const string& name, bool is_nullable, PrimitiveType ptype) {
  AST ast;
  SetFields(name, is_nullable, &ast);
  ast.mutable_p_ast()->set_type(ptype);
  ast.mutable_p_ast()->clear_val();
  return ast;
}

AST MakeInterval(const string& name, PrimitiveType ptype) {
  AST arg = MakePrimitive(kIntervalArg, true, ptype);
  std::vector<AST> args;
  args.emplace_back(arg);
  return MakeComposite(name, true, Operator::INTERVAL, args);
}

AST MakeList(const string& name, bool is_nullable, const AST& arg) {
  return MakeContainer(name, is_nullable, Operator::LIST, arg);
}

AST MakeSet(const string& name, bool is_nullable, const AST& arg) {
  return MakeContainer(name, is_nullable, Operator::SET, arg);
}

AST MakeContainer(const string& name, bool is_nullable, Operator op,
                  const AST& arg) {
  std::vector<AST> args;
  args.emplace_back(arg);
  AST ast = MakeComposite(name, is_nullable, op, args);
  ast.mutable_c_ast()->mutable_arg(0)->set_name(kContainerArg);
  return ast;
}

AST MakeTuple(const string& name, bool is_nullable,
              const std::vector<AST>& args) {
  return MakeComposite(name, is_nullable, Operator::TUPLE, args);
}

AST MakeComposite(const string& name, bool is_nullable, Operator op,
                  const std::vector<AST>& args) {
  AST ast;
  SetFields(name, is_nullable, &ast);
  ast.mutable_c_ast()->set_op(op);

  string tmp_err;
  AST* new_arg;
  for (const auto& arg : args) {
    CHECK(IsType(arg, &tmp_err), tmp_err);
    new_arg = ast.mutable_c_ast()->add_arg();
    *new_arg = arg;
  }
  return ast;
}

AST MakeDirectory() {
  return MakeList(kDirectory, true, MakeString(kFilePathPart, false));
}

AST MakeFile() {
  std::vector<AST> args;
  args.push_back(MakeDirectory());
  args.push_back(MakeString(kFilename, true));
  return MakeTuple(kFileTag, true, args);
}

AST MakeIPAddress() {
  return MakeString(kIPAddressTag, false);
}

AST MakeURL() {
  return MakeString(kURLTag, false);
}

}  // namespace type
}  // namespace ast
}  // namespace morphie
