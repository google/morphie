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

// A PlasoEvent proto (defined in third_party/logle/plaso_event.proto)
// represents data extracted from the output of Plaso (go/plaso). This file
// defines utilities for parsing input and populating PlasoEvent protos.
#ifndef LOGLE_PLASO_EVENT_H_
#define LOGLE_PLASO_EVENT_H_

#include "ast.pb.h"
#include "base/string.h"
#include "json/json.h"
#include "plaso_event.pb.h"

namespace tervuren {
// The 'plaso' namespace contains utilities for dealing with Plaso output.
namespace plaso {

// Converts a Unix-style Plaso filename into a File proto. This is not a generic
// filename parsing utility. Plaso normalizes filenames so that even a Windows
// filename is of the form C:/path/to/file.ext. This function also assumes that
//  - the file delimiter is '/'.
//  - a filename cannot be the empty string.
//  Examples.
//   - "" generates no filename and no directory.
//   - "/" generates no filename and the empty string as the directory.
//   - "filename.txt" generates a filename with no directory.
//   - "/usr/local/" generates the directory with path "usr", "local" but no
//     filename.
//   - "/foo/bar/baz.f" generates "foo", "bar" as 'directory().path()' and
//     "baz.f" as the filename.
File ParseFilename(const string& filename);

// Constructs an event proto from a JSON object generated by Timesketch.
PlasoEvent ParseJSON(const ::Json::Value& json_event);

// Return a PlasoEventGraph AST representing a file.
AST ToAST(const File& file);

// Return a string with the full path and filename of the file in 'file'.
string ToString(const File& file);

}  // namespace plaso
}  // namespace tervuren

#endif  // LOGLE_PLASO_EVENT_H_
