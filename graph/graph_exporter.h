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

// This file contains utilities for exporting a Logle LabeledGraph to a graph
// representation that can be used by an internal visualizer. The visualizer is
// currently experimental but once it is mature, will also be released.
#ifndef LOGLE_GRAPH_EXPORTER_H_
#define LOGLE_GRAPH_EXPORTER_H_

#include <string>

#include "base/string.h"
#include "graph/labeled_graph.h"
#include "third_party/logle/graph/ast.pb.h"
#include "third_party/logle/graph/graph_explorer.pb.h"

namespace morphie {

// A namespace containing visualization-related utilities.
namespace viz {

namespace ge = ::graph_explorer;
// A node identifier is a string that matches the regexp:
//  "[A-Za-z0-9.][A-Za-z0-9_./]*"
//
// Identifiers serve as a clustering mechanism. A common, slash-separated prefix
// defines one node in the hierarchy.
//
// An identifier function takes a tag and an AST that label a graph node or edge
// and returns a TensorFlow label. See ast.proto for more on ASTs.
using LabelFn = std::function<string(const string&, const AST&)>;

// The GraphExporter class below uses an internal map from LabeledGraph node ids
// to GraphDef node identifiers. A separate GraphExporter object has to be
// created for each graph that is to be exported.
class GraphExporter {
 public:
  // This constructor sets the default node label function, which is
  // GraphExporter::NodeLabel.
  GraphExporter(const LabeledGraph& graph);

  // This constructor uses the 'node_label' argument to customize the generation
  // of node labels. The caller must ensure that the label function generates a
  // syntactically correct TensorFlow graph label, as described in the
  // class-level comment.
  GraphExporter(const LabeledGraph& graph, const LabelFn& node_label);

  // Returns the tag and contents of the AST as a slash-delimited string.
  static string TextLabel(const string& tag, const AST& ast);
  // Returns the AST as an HTML string. Primary AST values are treated as plain
  // strings and composite values are represented as a table.
  static string HTMLLabel(const string& tag, const AST& ast);

  // Returns the TensorFlow graph for the internally stored LabeledGraph.
  ge::GraphDef Graph();

  // Returns a human-readable serialization of the GraphDef proto above.
  string GraphAsString();

 private:
  // Returns the node label as a text string followed by the node identifier
  // from the internal representation of the graph.
  string NodeName(NodeId node_id, const string& tag, const AST& ast);
  // Returns the label string for 'node_id' in 'graph'. Returns a cached string
  // if this label has been computed previously and generates a new label
  // otherwise.
  string FindOrAddName(NodeId node_id);
  // Returns a representation of 'node_id' and its predecessors for
  // visualization.
  ge::Node Node(NodeId node_id);

  const LabeledGraph& graph_;
  // The function used to generate node labels.
  LabelFn node_label_;
  // The map from LabeledGraph node identifiers to TensorFlow NodeDef names.
  unordered_map<NodeId, string> node_name_;
};  // class GraphExporter

}  // namespace viz
}  // namespace morphie
#endif  // LOGLE_GRAPH_EXPORTER_H_
