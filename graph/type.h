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

// This file contains functions for constructing ASTs that represent types.  See
// third_party/logle/ast.proto and third_party/logle/type_checker.h for
// details on types.
#ifndef LOGLE_TYPE_H_
#define LOGLE_TYPE_H_


#include "third_party/logle/graph/ast.pb.h"
#include "base/string.h"
#include "base/vector.h"

namespace morphie {
namespace ast {
namespace type {

// The Make[Type](name, is_nullable, ...) functions return an AST for a type
// with the 'name' and 'is_nullable' fields set to the given inputs.
//
// Return an AST for the null type.
AST MakeNull(const string& name);

// The functions below create primitive types.
AST MakeBool(const string& name, bool is_nullable);
AST MakeInt(const string& name, bool is_nullable);
AST MakeString(const string& name, bool is_nullable);
AST MakeTimestamp(const string& name, bool is_nullable);
AST MakePrimitive(const string& name, bool is_nullable, PrimitiveType ptype);

// This function returns an interval type with arguments of type 'ptype'. The
// interval is nullable and has nullable arguments.
//  - Requires that 'ptype' is ordered, which can be checked by calling
//    ast::IsOrdered(ptype).
AST MakeInterval(const string& name, PrimitiveType ptype);

// The functions below create composite types with 'arg' and 'args' containing
// the arguments to the type constructor.
//  - These functions require that 'arg' and 'args' contain types, which can
//    be checked by calling type::IsAType(arg).
AST MakeList(const string& name, bool is_nullable, const AST& arg);
AST MakeSet(const string& name, bool is_nullable, const AST& arg);
AST MakeContainer(const string& name, bool is_nullable, Operator op,
                  const AST& arg);
AST MakeTuple(const string& name, bool is_nullable, const vector<AST>& args);
AST MakeComposite(const string& name, bool is_nullable, Operator op,
                  const vector<AST>& args);

// The Make[Data] functions below generate predefined ASTs for commonly
// occurring types of data.
//
// A directory is a list of strings representing a path.
AST MakeDirectory();
// A file consists of a directory containing the file and a filename.
AST MakeFile();
// An IP address and a URL are represented as strings.
AST MakeIPAddress();
AST MakeURL();

}  // namespace type
}  // namespace ast
}  // namespace morphie

#endif  // LOGLE_TYPE_H_
