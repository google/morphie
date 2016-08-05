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

// An account access graph represents accesses to accounts by actors. There are
// two types of nodes: actor nodes and user nodes. An actor node is labelled
// with the actor's account, title and manager. A user node is labelled with a
// user account. An access edge goes from an actor to a user and is labelled
// with the number of accesses that were made.
//
// Status: The purpose of this graph is to demonstrate potential applications of
// graph-based log analysis. There is currently only support for construction
// and rudimentary visualization of this graph.
#ifndef THIRD_PARTY_LOGLE_ACCOUNT_ACCESS_GRAPH_H_
#define THIRD_PARTY_LOGLE_ACCOUNT_ACCESS_GRAPH_H_

#include <unordered_map>
#include <vector>

#include "third_party/logle/base/string.h"
#include "third_party/logle/graph_interface.h"
#include "third_party/logle/labeled_graph.h"
#include "third_party/logle/util/status.h"

namespace tervuren {

// The account access graph contains labels with the following names and types.
// The labels are all unique, meaning there can be at most one node with each
// label in the graph and at most one edge between two nodes with a given label.
// * Actor : tuple(ActorId : string, Title : string, Manager : string)
// * User : string
// * Access Edge : Count : int
// As currently implemented, if an account is an actor and a user, it will
// appear as two nodes in the graph.
//
// The account access graph must be initialized explicitly by calling Initialize
// before calling any other functions. Functions will crash if the graph is not
// initialized.
class AccountAccessGraph : public GraphInterface {
 public:
  AccountAccessGraph() : is_initialized_(false) {}

  // Initializes the graph. This function must be called before all other
  // functions in this class. Returns
  // - Status::OK - if a graph with the appropriate types was created.
  // - Status::INTERNAL - otherwise, with the reason accessible via the
  //   Status::error_message() function of the Status object.
  util::Status Initialize();

  // Functions for statistics about nodes and edges.
  // Statistics about nodes.
  int NumNodes() const;
  int NumLabeledNodes(const TaggedAST& label) const;
  // Statistics about edges.
  int NumEdges() const;
  int NumLabeledEdges(const TaggedAST& label) const;

  // Extract data from 'fields' and add nodes and edges to the graph for a
  // single access. The arguments are:
  // - 'fields' - a vector of fields derived from the input.
  // - 'field_index' - a map from field names to their position in 'fields'.
  // Requires that
  // * 'field_index' is not empty.
  // * 'fields' is not empty.
  // * for each pair (name, index) in 'field_index', 'index' is a valid index of
  //   'fields'.
  // This function does not perform data validation, meaning that every entry in
  // 'fields' is assumed to be valid. If, for example, an empty string is used
  // to represent an unknown user, there will be a user node in the graph
  // labelled with the empty string.
  void ProcessAccessData(const unordered_map<string, int>& field_index,
                         const vector<string>& fields);

  // Return a representation of the graph in Graphviz DOT format.
  string ToDot() const;

 private:
  // The functions below create each of the three types of labels in the graph.
  TaggedAST MakeActorLabel(const unordered_map<string, int>& field_index,
                           const vector<string>& fields);
  TaggedAST MakeUserLabel(const unordered_map<string, int>& field_index,
                          const vector<string>& fields);
  TaggedAST MakeEdgeLabel(const unordered_map<string, int>& field_index,
                          const vector<string>& fields);

  bool is_initialized_;
  LabeledGraph graph_;
};  // class AccountAccessGraph

}  // namespace tervuren

#endif  // THIRD_PARTY_LOGLE_ACCOUNT_ACCESS_GRAPH_H_
