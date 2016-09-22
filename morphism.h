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

// A graph transformation generates an output graph from an input graph. For
// interactive visualization and graph summarization, it is useful to be able to
// relate nodes and edges in the input graph and output graphs.
//
// Example 1. Consider a graph with edges {1 --> 1, 1 -> 2} and the graph
// {1 --> 1} obtained by deleting the node 2. The node '1' and its self-edge map
// to the output graph but the node '2' and its incoming edge do not exist in
// the output.
//
// Example 2. Consider a graph with edges {1 --> 2, 2 -> 1} and the graph
// {a --> a} obtained by merging the nodes 1 and 2, while preserving their edge
// relationships. The input nodes '1' and '2' map to the output node 'a' and
// both input edges map to the same output edge. If self-loops are ommitted in
// the output, the nodes '1' and'2' will map to 'a' but the edges between them
// will not map to anything.
//
// This file defines the Morphism class for tracking relationships between nodes
// and edges in input and output graphs. The main feature of Morphisms is that
// they compose, so one can apply a series of transformations to an input graph
// and only obtain the relationship between the first and large graph generated
// by these transformations.
#ifndef THIRD_PARTY_LOGLE_MORPHISM_H_
#define THIRD_PARTY_LOGLE_MORPHISM_H_

#include <unordered_map>

#include "labeled_graph.h"
#include "third_party/logle/ast.pb.h"

namespace tervuren {
namespace graph {

// A morphism, with lower-case 'm', is a pair of partial functions from the
// nodes and edges of an input graph to the nodes an edges of the output graph.
// Some nodes and edges in the input graph may have no corresponding elements in
// the output graph, which is why the functions are partial.
//
// The Morphism class provides helper functions for creating and manipulating
// morphisms.
class Morphism {
 public:
  // A Morphism requires a non-null pointer and will not own the input graph.
  explicit Morphism(const LabeledGraph* graph) : input_graph_(*graph) {}

  bool HasOutputGraph() { return output_graph_ != nullptr; }
  // Functions that return the input and output graphs.
  const LabeledGraph& Input() const { return input_graph_; }
  const LabeledGraph& Output() const { return *output_graph_; }
  LabeledGraph* MutableOutput() { return output_graph_.get(); }
  // Returns and gives up ownership of the output graph and clears the internal
  // maps between input and output nodes.
  std::unique_ptr<LabeledGraph> TakeOutput();

  // Creates a new output graph that has the same node and edge types as the
  // input graph. An output graph that already exists will no longer be
  // accessible.
  void CloneInputType();

  // Returns the id of the output node that the input node maps to in the
  // morphism. Adds a new node to the output graph if no such node exists.
  NodeId FindOrMapNode(NodeId input_node, TaggedAST label);

  // Composes this morphism with the input and takes ownership of the output
  // graph in the input morphism. The output graph that existed before
  // composition cannot be access after the composition.
  util::Status ComposeWith(Morphism* morphism);

 private:
  const LabeledGraph& input_graph_;
  std::unordered_map<NodeId, NodeId> node_map_;
  std::unordered_map<NodeId, std::unordered_set<NodeId>> node_preimage_;
  std::unique_ptr<LabeledGraph> output_graph_;
};  // class Morphism

}  // namespace graph
}  // namespace tervuren
#endif  // THIRD_PARTY_LOGLE_MORPHISM_H_
