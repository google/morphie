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

// This file contains constants used in the analysis of Curio streams.
#ifndef LOGLE_CURIO_DEFS_H_
#define LOGLE_CURIO_DEFS_H_

namespace logle {
// The 'curio' namespace encloses definitions used to analyze Curio streams.
namespace curio {

// The field name for children of a stream.
extern const char kChildrenField[];

// Tags used in graph labels.
extern const char kStreamTag[];
extern const char kStreamIdTag[];
extern const char kStreamNameTag[];
extern const char kDependentTag[];

}  // namespace curio
}  // logle

#endif  // LOGLE_CURIO_DEFS_H_
