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

// This file contains utilities for analyzing graphs.
#ifndef THIRD_PARTY_LOGLE_GRAPH_ANALYZER_H_
#define THIRD_PARTY_LOGLE_GRAPH_ANALYZER_H_

#include <map>
#include "labeled_graph.h"

namespace tervuren {

// Contains methods for analyzing graphs and structures on graphs.
namespace graph_analyzer {

// Takes a partition of the nodes of a graph and returns a refinement of that
// partition.
// Example: Consider the following scenario - threads T0 and T1 both spawn a
// similar series of events. These events are similar so both sets get
// partitioned as E. The refinement would split E into E0 and E1 so that we
// have T0 -> E0, T1 -> E1, instead of
// T0   T1
//   \ /
//    E
//
// The relational coarsest partition problem is as follows: Given a partition P
// of a set U and a binary relation E on U, find the coarsest refinement Q of P
// such that Q is stable with respect to E. A partition Q is stable with respect
// to E if for all blocks B_1, B_2 of Q, either B_1 is contained in, or disjoint
// from E^-1(B_2) (where E^-1(B_2) is the preimage of B_2).
// Paige, Robert; Tarjan, Robert E. (1987), "Three partition refinement
// algorithms", SIAM Journal on Computing 16 (6): 973–989
std::map<NodeId, int> RefinePartition(const LabeledGraph& graph,
                                      const std::map<NodeId, int>& partition);
}  // namespace graph_analyzer

}  // namespace tervuren


#endif  // THIRD_PARTY_LOGLE_GRAPH_ANALYZER_H_
