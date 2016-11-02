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

// The 'ast' namespace contains utilities that operate on Abstract Syntax Tree
// (AST) protos. See third_party/logle/ast.proto for a definition of AST
// protos. An AST proto that represents a type or a value must satisfy
// additional constraints. The functions in the 'ast' namespace impose no
// constraints on the protos.
//
//  Example. Let 'int_asts' be the list of nullable integers: (1, null, 3).
//
//  - ast::ToString(int_asts, PrintConfig(PrintOption::kType)) returns
//   "list( int?, int?, int?)".
//  - ast::ToString(int_asts, PrintConfig(PrintOption::kValue)) returns
//    "( 1 , null, 3 )".
//  - ast::ToString(int_asts, PrintConfig(PrintOption::kTypeAndValue)) returns
//    "list( 1 : int?, null : int?, 3 : int?)".
//
// Boolean, integer and string values are printed in the standard manner.
// Timestamps, which are stored as microseconds since the Unix epoch, are
// printed in RFC3339 format.
//
// These methods are not guaranteed to be thread safe.
#ifndef LOGLE_AST_H_
#define LOGLE_AST_H_

#include "ast.pb.h"
#include "base/string.h"

namespace morphie {
namespace ast {

// Constants used by AST utilities.
extern const char kDirectory[];
extern const char kFilename[];
extern const char kFilePathPart[];
extern const char kFileTag[];
extern const char kIPAddressTag[];
extern const char kTimeTag[];
extern const char kURLTag[];
extern const char kPrecedesTag[];
extern const char kUsesTag[];

// A PrintOption allows for choosing some combination of name, type and value to
// display in pretty printed output. The kName, kType and kValue option are
// represented by bitvectors for different powers of 2 and the other options are
// obtained by bit masks. The options kNameAndType and kValue are used when
// displaying the contents of labelled graphs, while kType an kTypeAndValue are
// used to generate error messages during type checking.
enum class PrintOption : int {
  kName = 1 << 0,
  kType = 1 << 1,
  kValue = 1 << 2,
  kNameAndType = kName | kType,
  kTypeAndValue = kType | kValue,
  kNameAndValue = kName | kValue,
  kAll = kName | kType | kValue,
};

// A PrintConfig determines how an AST is printed. It specifies the PrintOption
// to use and the delimiters that enclose and separate AST elements. The default
// configuration is to only show values, to enclose the AST in parentheses and
// use a comma to separate arguments.
//
// Example.
//   AST interval_ast;
//   // Populate the interval with lower bound 0, and upper bound 5.
//   EXPECT_EQ("(0, 5)", ToString(ast, PrintConfig()));
//   PrintConfig config("[", "]", " - ");
//   EXPECT_EQ("[0 - 5]", ToString(ast, config));
class PrintConfig {
 public:
  PrintConfig()
      : open_("("), close_(")"), sep_(", "), opt_(PrintOption::kValue) {}

  PrintConfig(PrintOption opt)
      : open_("("), close_(")"), sep_(", "), opt_(opt) {}

  PrintConfig(const string& open, const string& close, const string& sep,
              PrintOption opt)
      : open_(open), close_(close), sep_(sep), opt_(opt) {}

  // Functions for retrieving and setting configuration options.
  string open() const { return open_; }
  void set_open(const string& open) { open_ = open; }

  string close() const { return close_; }
  void set_close(const string& close) { close_ = close; }

  string sep() const { return sep_; }
  void set_sep(const string& sep) { sep_ = sep; }

  PrintOption opt() const { return opt_; }
  void set_opt(PrintOption opt) { opt_ = opt; }

 private:
  string open_;   // First character when printing a composite AST.
  string close_;  // Last character when printing a composite AST.
  string sep_;    // Separator to use between arguments of a composite AST.
  PrintOption opt_;
};

// Returns true if 'ast' contains no syntax tree. A null AST is different from
// the AST for a null type because the 'is_nullable' and 'name' fields are set
// in the AST for a type. The null AST is also different from an AST for a null
// value of primitive type. For example, an AST for a null string value will
// contain type information but no value.
bool IsNull(const AST& ast);

// The Is[TypeFamily] functions return true if the root of 'ast' is the operator
// for the type family in the function name. Similar to the IsNull check above,
// these are checks of the structure of the AST and do not check if the AST
// contains enough information to represent a type or a value of a given type.
bool IsBool(const AST& ast);
bool IsInt(const AST& ast);
bool IsString(const AST& ast);
bool IsTimestamp(const AST& ast);
bool IsContainer(const AST& ast);
bool IsList(const AST& ast);
bool IsSet(const AST& ast);
bool IsTuple(const AST& ast);

// The ToString methods pretty print the contents of an AST to a string. The
// print config argument determines which contents of the AST to print. See the
// example at the top of the file for how to use these methods.
string ToString(const TaggedAST& ast, const PrintConfig& config);
string ToString(const TaggedAST& ast, const PrintConfig& config);
string ToString(const AST& ast, const PrintConfig& config);
string ToString(const AST& ast, const PrintConfig& config);
string ToString(const PrimitiveAST& p_ast, const PrintConfig& config);
string ToString(const PrimitiveValue& val);
string ToString(const PrimitiveType& type);
string ToString(const Operator& op);

// Returns only the root of the abstract syntax tree as a string. Unlike the
// methods above, this function takes a PrintOption and not a PrintConfig as an
// argument because the root of an AST is serialized without delimiters. For
// primitive types, ToStringRoot and ToString return the same string.
//
//  Eg. Let 'ast' be the AST in the first example.
//  ToStringRoot(ast, opt) returns "list" if 'opt' is kType or kTypeAndValue
//  and returns the empty string otherwise.
string ToStringRoot(const AST& ast, PrintOption opt);

}  // namespace ast
}  // namespace morphie

#endif  // LOGLE_AST_H_
