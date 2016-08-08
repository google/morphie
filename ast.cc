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

// This file contains functions for checking properties of AST protos and for
// pretty printing those protos. The printing functions recursively walk the AST
// and return the type stored at each node. Most functions also have an argument
// 'show_value'. The value branch of a PrimitiveAST is only visited when this
// argument is true.
#include "ast.h"

#include <boost/algorithm/string/join.hpp>  // NOLINT

#include "base/vector.h"
#include "util/string_utils.h"
#include "util/time_utils.h"

namespace tervuren {
namespace ast {

namespace {

const char kNullStr[] = "null";
const char kNullOpStr[] = "?";
const char kTagStr[] = "tag";

// Returns 'true' if 'b' includes all options in 'a'.
bool Includes(PrintOption a, PrintOption b) {
  return (static_cast<int>(a) & static_cast<int>(b)) == static_cast<int>(a);
}

// Returns true if 'ast' has the primitive type in the second argument.
bool IsPrimitive(const AST& ast, PrimitiveType type) {
  return ast.has_p_ast() && (ast.p_ast().type() == type);
}

// Returns a single whitespace string if the name and remaining output will be
// non-empty. The remaining output will be non-empty if it includes a type or a
// value because empty types and values serialize as "null".
string GetNameSeparator(const string& name, PrintOption opt) {
  if (name != "" && (Includes(PrintOption::kNameAndType, opt) ||
                     Includes(PrintOption::kNameAndValue, opt))) {
    return " ";
  }
  return "";
}

// Returns the type of an AST.
string ToTypeString(const AST& ast, PrintOption opt) {
  if (!Includes(PrintOption::kType, opt)) {
    return "";
  }
  if (IsNull(ast)) {
    return kNullStr;
  } else if (ast.has_p_ast()) {
    return ToString(ast.p_ast().type());
  } else if (ast.has_c_ast()) {
    return ToString(ast.c_ast().op());
  }
  return "";
}

// Returns the value in an AST.
string ToValueString(const AST& ast, PrintOption opt) {
  if (!Includes(PrintOption::kValue, opt)) {
    return "";
  }
  if (IsNull(ast)) {
    return kNullStr;
  } else if (ast.has_p_ast()) {
    if (ast.p_ast().has_val()) {
      return ToString(ast.p_ast().val());
    } else {
      return kNullStr;
    }
  }
  return "";
}

}  // namespace

// Constants for graph node types.
const char kDirectory[] = "Directory";
const char kFilename[] = "Filename";
const char kFilePathPart[] = "File Path";
const char kFileTag[] = "File";
const char kIPAddressTag[] = "IP-Address";
const char kTimeTag[] = "Time";
const char kURLTag[] = "URL";
// Constants for graph edge types.
const char kPrecedesTag[] = "Precedes";
const char kUsesTag[] = "Uses";

bool IsNull(const AST& ast) {
  return ast.node_case() == AST::NodeCase::NODE_NOT_SET;
}

bool IsBool(const AST& ast) { return IsPrimitive(ast, PrimitiveType::BOOL); }

bool IsInt(const AST& ast) { return IsPrimitive(ast, PrimitiveType::INT); }

bool IsString(const AST& ast) {
  return IsPrimitive(ast, PrimitiveType::STRING);
}

bool IsTimestamp(const AST& ast) {
  return IsPrimitive(ast, PrimitiveType::TIMESTAMP);
}

bool IsContainer(const AST& ast) {
  return (ast.has_c_ast() && (ast.c_ast().op() == Operator::LIST ||
                              ast.c_ast().op() == Operator::SET ||
                              ast.c_ast().op() == Operator::TUPLE));
}

bool IsList(const AST& ast) {
  return (ast.has_c_ast() && ast.c_ast().op() == Operator::LIST);
}

bool IsSet(const AST& ast) {
  return (ast.has_c_ast() && ast.c_ast().op() == Operator::SET);
}

bool IsTuple(const AST& ast) {
  return (ast.has_c_ast() && ast.c_ast().op() == Operator::TUPLE);
}

// The string constant kTagStr is treated as the type name for a tag and
// ast.tag() is the value. The print option determines which of these is added
// as a prefix when pretty printing 'ast.ast()'.
// If Includes(PrintOption::kType, opt) is true, the output should include type
// information.
string ToString(const TaggedAST& ast, const PrintConfig& config) {
  string type_str = Includes(PrintOption::kType, config.opt()) ? kTagStr : "";
  string type_sep = Includes(PrintOption::kValue, config.opt()) ? " : " : "";
  string tag_str = ast.has_tag() ? ast.tag() : "";
  string ast_str = ast.has_ast() ? ToString(ast.ast(), config) : kNullStr;
  return util::StrCat(type_str, type_sep, tag_str, " :: ", ast_str);
}

string ToString(const AST& ast, const PrintConfig& config) {
  string root = ToStringRoot(ast, config.opt());
  if (Includes(config.opt(), PrintOption::kName) || !ast.has_c_ast()) {
    return root;
  }
  vector<string> arg_str;
  if (ast.c_ast().arg_size() == 0) {
    arg_str.emplace_back(kNullStr);
  } else {
    arg_str.emplace_back(ToString(ast.c_ast().arg(0), config));
  }
  for (int i = 1; i < ast.c_ast().arg_size(); ++i) {
    arg_str.emplace_back(ToString(ast.c_ast().arg(i), config));
  }
  return util::StrCat(root, config.open(),
                      boost::algorithm::join(arg_str, config.sep()),
                      config.close());
}

string ToString(const PrimitiveAST& p_ast, const PrintConfig& config) {
  AST ast;
  *ast.mutable_p_ast() = p_ast;
  return ToStringRoot(ast, config.opt());
}

string ToString(const PrimitiveValue& val) {
  switch (val.val_case()) {
    case PrimitiveValue::ValCase::kBoolVal:
      return val.bool_val() ? "true" : "false";
    case PrimitiveValue::ValCase::kIntVal:
      return std::to_string(val.int_val());
    case PrimitiveValue::ValCase::kStringVal:
      return val.string_val();
    case PrimitiveValue::ValCase::kTimeVal:
      return util::UnixMicrosToRFC3339(val.time_val());
    case PrimitiveValue::ValCase::VAL_NOT_SET:
      return kNullStr;
  }
}

string ToString(const PrimitiveType& type) {
  switch (type) {
    case PrimitiveType::BOOL:
      return "bool";
    case PrimitiveType::INT:
      return "int";
    case PrimitiveType::STRING:
      return "string";
    case PrimitiveType::TIMESTAMP:
      return "timestamp";
  }
}

string ToString(const Operator& op) {
  switch (op) {
    case Operator::INTERVAL:
      return "interval";
    case Operator::LIST:
      return "list";
    case Operator::SET:
      return "set";
    case Operator::TUPLE:
      return "tuple";
  }
}

// Returns a string whose output with the print option kAll is:
//   [name] [type] : [value] for primitive ASTs and
//   [name] [type] for composite ASTs.
// A subset of this output will appear depending on the options chosen. The
// separator " : " is added if the output contains both a type and a value.
string ToStringRoot(const AST& ast, PrintOption opt) {
  string name = "";
  if (Includes(PrintOption::kName, opt) && ast.has_name()) {
    name = ast.name();
  }
  string name_sep = GetNameSeparator(name, opt);
  string type_str = ToTypeString(ast, opt);
  string null_op = "";
  if (Includes(PrintOption::kType, opt) && ast.has_is_nullable() &&
      ast.is_nullable()) {
    null_op = kNullOpStr;
  }
  string type_sep = "";
  if (!ast.has_c_ast() && Includes(PrintOption::kTypeAndValue, opt)) {
    type_sep = " : ";
  }
  string val_str = ToValueString(ast, opt);
  return util::StrCat(name, name_sep, type_str, null_op, type_sep, val_str);
}

}  // namespace ast
}  // namespace tervuren
