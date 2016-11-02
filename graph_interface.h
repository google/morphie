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
//
#ifndef LOGLE_GRAPH_INTERFACE_H_
#define LOGLE_GRAPH_INTERFACE_H_

#include "base/string.h"
#include "util/status.h"

namespace morphie {

// The GraphInterface class is an abstract class that specifies the common API
// for instantiations of the labeled graph.

class GraphInterface {
 public:

  virtual ~GraphInterface() {}

  // Initializes the graph. This function must be called before all other
  // functions in this class. Returns
  // - Status::OK - if a graph with the appropriate types was created.
  // - Status::INTERNAL - otherwise, with the reason accessible via the
  //   Status::error_message() function of the Status object.
  virtual util::Status Initialize() = 0;

  // Functions for statistics about nodes and edges.
  virtual int NumNodes() const = 0;
  virtual int NumEdges() const = 0;

  // Return a representation of the graph in Graphviz DOT format.
  virtual string ToDot() const = 0;
};  // class GraphInterface

}  // namespace morphie
#endif  // LOGLE_GRAPH_INTERFACE_H_

