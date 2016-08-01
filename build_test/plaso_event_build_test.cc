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

// Test building and linking of the PlasoEvent proto and related utilities. The
// executable should print a proto representation of a filename.
#include <iostream>

#include "plaso_event.h"
#include "plaso_event.pb.h"

int main(int argc, char **argv) {
  logle::File file = logle::plaso::ParseFilename("/tmp/build/test/file.txt");
  std::cout << file.DebugString() << std::endl;
}
