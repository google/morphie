// The methods in this file check if an AST represents a type and if an AST
// representing a value respects a given type. Types and values have an
// inductive structure given by the grammar in third_party/logle/ast.proto. The
// methods that check if an AST is a type walk the tree and check that each
// operator node has the right number of arguments. The methods to check if a
// value respects a type simultaneously walk the type and value ASTs and check
// if the node in the value AST has the appropriate operator and number of
// arguments as specified by the type AST.
#include "third_party/logle/type_checker.h"

#include <algorithm>
#include <iostream>
#include <utility>

#include "third_party/logle/ast.h"
#include "third_party/logle/util/logging.h"
#include "third_party/logle/util/string_utils.h"
#include "third_party/logle/value_checker.h"

namespace third_party_logle {
namespace ast {
namespace type {

namespace {

const char kIntervalErr[] = "The interval type constructor must have "
    "one argument which is a primitive type.";
const char kNoTagErr[] = "There is no type defined for the tag : ";
const char kOneArgErr[] = "This type constructor must have one argument.";

// In the methods below, 'path' represents the ancestors of a node in an AST or
// CompositeAST argument. The value of 'path' is used for error reporting.

// Returns an error message that 'type' and 'val' have different types.  This is
// a pretty printing routine that assumes that the root nodes of 'ast' and 'val'
// have different types and does no type checking.
void GetTypeMismatch(const AST& type, const AST& val, const string& path,
                     string* err);

// The methods below have the form IsXX(ast, path, err). They return:
//   - 'true' if 'ast' represents a type.
//   - 'false' otherwise and write a human readable error message to '*err'.
bool IsTypeInternal(const AST& ast, const string& path, string* err);
bool IsAnInterval(const CompositeAST& ast, const string& path, string* err);
bool IsAContainer(const CompositeAST& ast, const string& path, string* err);
bool IsATuple(const CompositeAST& ast, const string& path, string* err);
bool IsNullable(const AST& ast, const string& path, string* err);

// The methods below have the form IsXX(type, val, path, err). They return:
//   - 'true' if 'val' respects 'type'.
//   - 'false' otherwise and write a human readable error message to '*err'.
bool IsTypedInternal(const AST& type, const AST& val, const string& path,
                     string* err);
bool IsPrimitive(const AST& type, const PrimitiveAST& p_ast, const string& path,
                 string* err);
bool IsComposite(const AST& type, const CompositeAST& cval, const string& path,
                 string* err);
bool IsInterval(const AST& type, const CompositeAST& cval, const string& path,
                string* err);
bool IsContainer(const AST& type, const CompositeAST& cval, const string& path,
                 string* err);
bool IsTuple(const AST& type, const CompositeAST& cval, const string& path,
             string* err);

void GetTypeMismatch(const AST& type, const AST& val, const string& path,
                     string* err) {
  string field_name = util::StrCat(path, ".", type.name());
  string expected_type = ast::ToStringRoot(type, PrintOption::kType);
  string actual_type = ast::ToStringRoot(val, PrintOption::kType);
  *err = util::StrCat("The field ", field_name, " should have type:\n  ",
                      expected_type, "\nbut has type :\n  ", actual_type);
}

bool IsTypeInternal(const AST& ast, const string& path, string* err) {
  if (!ast.has_name()) {
    *err = util::StrCat("A sub-field of ", path, " has no name.");
    return false;
  }
  string new_path = util::StrCat(path, ".", ast.name());
  if (!ast.has_is_nullable()) {
    *err = util::StrCat("The nullable flag for ", new_path, " is missing.");
    return false;
  }
  if (ast::IsNull(ast)) {
    if (!ast.is_nullable()) {
      *err = util::StrCat("The AST ", new_path,
                          " must have a type or be nullable.");
      return false;
    }
    return true;
  }
  if (ast.has_p_ast()) {
    return true;
  }
  switch (ast.c_ast().op()) {
    case Operator::INTERVAL:
      return IsAnInterval(ast.c_ast(), new_path, err);
    case Operator::LIST:
      return IsAContainer(ast.c_ast(), new_path, err);
    case Operator::TUPLE:
      return IsATuple(ast.c_ast(), new_path, err);
    case Operator::SET:
      return IsAContainer(ast.c_ast(), new_path, err);
  }
}

bool IsAnInterval(const CompositeAST& ast, const string& path, string* err) {
  CHECK(ast.op() == Operator::INTERVAL, "");
  if (ast.arg_size() == 1) {
    return ast.arg(0).has_p_ast();
  }
  *err = util::StrCat("The type of ", path, " is malformed.", kIntervalErr);
  return false;
}

bool IsAContainer(const CompositeAST& ast, const string& path, string* err) {
  CHECK(ast.op() != Operator::INTERVAL, "");
  CHECK(ast.op() != Operator::TUPLE, "");
  if (ast.arg_size() != 1) {
    string op_str = ast::ToString(ast.op());
    *err =
        util::StrCat("The field ", path, " has type ", op_str, ".", kOneArgErr);
    return false;
  }
  return IsTypeInternal(ast.arg(0), path, err);
}

bool IsATuple(const CompositeAST& ast, const string& path, string* err) {
  CHECK(ast.op() == Operator::TUPLE, "");
  if (ast.arg_size() == 0) {
    string op_str = ast::ToString(ast.op());
    *err = util::StrCat("The type constructor ", path,
                        " requires at least one argument.");
    return false;
  }
  return std::all_of(ast.arg().begin(), ast.arg().end(),
                     [&path, err](const AST& arg) {
                       return IsTypeInternal(arg, path, err);
                     });
}

bool IsNullable(const AST& ast, const string& path, string* err) {
  if (!ast.is_nullable()) {
    *err = util::StrCat("The field ", path, ".", ast.name(),
                        " must not be empty.");
    return false;
  }
  return true;
}

// This code invokes IsPrimitive(type, val.p_ast(),..) and
// IsComposite(type, val.c_ast(),..) with 'type' as the first argument rather
// than type.p_ast() or type.c_ast() because the value of type.is_nullable()
// is required to determine if an empty value AST is typed.
bool IsTypedInternal(const AST& type, const AST& val, const string& path,
                     string* err) {
  if (ast::IsNull(type) && ast::IsNull(val)) {
    return true;
  }
  if (type.has_p_ast() && val.has_p_ast()) {
    return IsPrimitive(type, val.p_ast(), path, err);
  }
  if (type.has_c_ast() && val.has_c_ast()) {
    return IsComposite(type, val.c_ast(), path, err);
  }
  GetTypeMismatch(type, val, path, err);
  return false;
}

bool IsPrimitive(const AST& type, const PrimitiveAST& pval, const string& path,
                 string* err) {
  CHECK(type.has_p_ast(), "");
  if (type.p_ast().type() != pval.type()) {
    AST val;
    val.mutable_p_ast()->set_type(pval.type());
    GetTypeMismatch(type, val, path, err);
    return false;
  }
  if (!pval.has_val()) {
    return IsNullable(type, path, err);
  }
  bool is_value = value::IsPrimitive(type.p_ast().type(), pval.val());
  if (!is_value) {
    string type_str = ast::ToString(type.p_ast().type());
    string val_str = ast::ToString(type.p_ast().val());
    *err = util::StrCat("The field  ", path, ".", type_str,
                        " has the wrong type.");
  }
  return is_value;
}

bool IsComposite(const AST& type, const CompositeAST& cval, const string& path,
                 string* err) {
  CHECK(type.has_c_ast(), "");
  if (type.c_ast().op() != cval.op()) {
    AST val;
    val.mutable_c_ast()->set_op(cval.op());
    GetTypeMismatch(type, val, path, err);
    return false;
  }
  if (cval.arg_size() <= 0) {
    return IsNullable(type, path, err);
  }
  string new_path = util::StrCat(path, ".", type.name());
  switch (type.c_ast().op()) {
    case Operator::INTERVAL:
      return IsInterval(type, cval, new_path, err);
    case Operator::LIST:
      return IsContainer(type, cval, new_path, err);
    case Operator::TUPLE:
      return IsTuple(type, cval, new_path, err);
    case Operator::SET:
      return IsContainer(type, cval, new_path, err);
  }
}

// An interval value must have zero or two arguments. This method is only used
// when there are two arguments.
bool IsInterval(const AST& type, const CompositeAST& cval, const string& path,
                string* err) {
  CHECK(type.c_ast().op() == Operator::INTERVAL, "");
  if (cval.arg_size() != 2) {
    *err = util::StrCat("The interval ", path, " has ",
                        std::to_string(cval.arg_size()),
                        " arguments but should have two.");
    return false;
  }
  string new_path = util::StrCat(path, ".", type.name());
  bool is_typed =
      IsTypedInternal(type.c_ast().arg(0), cval.arg(0), new_path, err);
  if (is_typed) {
    is_typed = IsTypedInternal(type.c_ast().arg(0), cval.arg(1), new_path, err);
  }
  return is_typed;
}

bool IsContainer(const AST& type, const CompositeAST& cval, const string& path,
                 string* err) {
  const AST& arg_type = type.c_ast().arg(0);
  return std::all_of(cval.arg().begin(), cval.arg().end(),
                     [&arg_type, &path, err](const AST& arg) {
                       return IsTypedInternal(arg_type, arg, path, err);
                     });
}

bool IsTuple(const AST& type, const CompositeAST& cval, const string& path,
             string* err) {
  CHECK(type.c_ast().op() == Operator::TUPLE, "");
  if (cval.arg_size() != type.c_ast().arg_size()) {
    *err = util::StrCat(path, " has ", std::to_string(cval.arg_size()),
                        " instead of ", std::to_string(type.c_ast().arg_size()),
                        ".");
    return false;
  }
  bool is_typed = true;
  for (int i = 0; i < cval.arg_size() && is_typed; ++i) {
    is_typed = IsTypedInternal(type.c_ast().arg(i), cval.arg(i), path, err);
  }
  return is_typed;
}

}  // namespace

bool IsType(const AST& ast, string* err) {
  CHECK(err != nullptr, "");
  err->clear();
  return IsTypeInternal(ast, "", err);
}

bool AreTypes(const Types& types, string* err) {
  CHECK(err != nullptr, "");
  err->clear();
  for (const auto& pair : types) {
    if (!IsTypeInternal(pair.second, "", err)) {
      return false;
    }
  }
  return true;
}

string ToString(const Types& types) {
  string str;
  for (const auto& pair : types) {
    util::StrAppend(&str, pair.first, " :: ",
                    ast::ToString(pair.second, PrintOption::kType), "\n");
  }
  return str;
}

bool IsTyped(const AST& type, const AST& val, string* err) {
  CHECK(err != nullptr, "");
  err->clear();
  CHECK(IsTypeInternal(type, "", err), *err);
  return IsTypedInternal(type, val, "", err);
}

bool IsTyped(const Types& types, const TaggedAST& val, string* err) {
  CHECK(err != nullptr, "");
  err->clear();
  auto type_it = types.find(val.tag());
  if (type_it == types.end()) {
    *err = util::StrCat(kNoTagErr, val.tag());
    return false;
  }
  CHECK(IsTypeInternal(type_it->second, "", err), *err);
  if (!val.has_ast()) {
    return IsNullable(type_it->second, "", err);
  }
  return IsTypedInternal(type_it->second, val.ast(), "", err);
}

}  // namespace type
}  // namespace ast
}  // namespace third_party_logle
