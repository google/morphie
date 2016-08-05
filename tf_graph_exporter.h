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

// TensorFlow is a library that uses data flow graphs to represent numerical
// computation. The TensorFlow Graph Explorer is a browser application that
// allows for visualization and interactive exploration of TensorFlow graphs.
// This file contains utilities for exporting a Logle LabeledGraph to the
// GraphDef proto used by the TensorFlow Graph Explorer.
#ifndef THIRD_PARTY_LOGLE_TF_GRAPH_EXPORTER_H_
#define THIRD_PARTY_LOGLE_TF_GRAPH_EXPORTER_H_

#include <string>

#include "third_party/logle/ast.pb.h"
#include "base/string.h"
#include "labeled_graph.h"
#include "third_party/tensorflow/core/framework/graph.pb.h"

namespace tervuren {

// A TensorFlow node label is a string that matches the regexp
//  "[A-Za-z0-9.][A-Za-z0-9_./]*"
// as specified in the documentation of the NodeDef message in the file
//   tensorflow/core/framework/graph.proto
// in the TensorFlow source code. Nodes are clustered by slash-separated
// strings.
//
// A TensorFlow labeling function takes a tag and an AST that label a graph node
// or edge and returns a TensorFlow label. See ast.proto for more on ASTs.
using TFLabelFn = std::function<string(const string&, const AST&)>;

// The TFGraphExporter uses an internal map from LabeledGraph node ids to
// node names in the TensorFlow GraphDef proto. A separate TFGraphExporter has
// to be created for each graph that is to be exported.
class TFGraphExporter {
 public:
  // This constructor sets the default node label function, which is
  // TFGraphExporter::NodeLabel.
  TFGraphExporter(const LabeledGraph& graph);

  // This constructor uses the 'node_label' argument to customize the generation
  // of node labels. The caller must ensure that the label function generates a
  // syntactically correct TensorFlow graph label, as described in the
  // class-level comment.
  TFGraphExporter(const LabeledGraph& graph, const TFLabelFn& node_label);

  // These functions return the label string for a node. They are made available
  // for use in defining custom label generation functions.
  static string NodeLabel(const string& tag, const AST& ast);
  static string NodeLabelWithId(NodeId node_id, const string& tag,
                                const AST& ast);

  // Returns the TensorFlow graph for the internally stored LabeledGraph.
  tensorflow::GraphDef TFGraph();

  // Returns a human-readable serialization of the GraphDef proto above.
  string TFGraphAsString();

 private:
  // Returns the label string for 'node_id' in 'graph'. Returns a cached string
  // if this label has been computed previously and generates a new label
  // otherwise.
  string FindOrAddName(NodeId node_id);

  // Returns the TensorFlow NodeDef representation of 'node_id'.
  tensorflow::NodeDef TFNode(NodeId node_id);

  const LabeledGraph& graph_;
  // The function used to generate node labels.
  TFLabelFn node_label_;
  // The map from LabeledGraph node identifiers to TensorFlow NodeDef names.
  unordered_map<NodeId, string> node_name_;
};  // class TFGraphExporter

}  // namespace tervuren
#endif  // THIRD_PARTY_LOGLE_TF_GRAPH_EXPORTER_H_
