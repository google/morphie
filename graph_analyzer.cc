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

#include <memory>

#include "ast.h"
#include "graph_analyzer.h"

using std::list;
using std::map;
using std::set;
using std::shared_ptr;

namespace tervuren {

namespace graph_analyzer {

namespace {

// SuperBlock is defined below.
struct SuperBlock;

// A 'Block' is a block of a partition. The block contains a set of NodeId's.
// In addition the block maintains several records about where it is located
// within other objects.
// 'parent' is a weak_ptr to the SuperBlock containing it. A weak_ptr is used
// here instead of a shared_ptr because this would cause a cycle in shared_ptr
// dependence, leading to memory leaks.
// 'partitn_loc' is an iterator to the block's location in 'blocks' Used for
// quick deletion from 'blocks'.
// 'parent_loc' is an iterator to the block's location in the parent's
// list of children. Used for quick deletion from the parent.
struct Block {
 public:
  Block() : elements() {}
  explicit Block(NodeId id) : elements({id}) {}
  set<NodeId> elements;
  std::weak_ptr<SuperBlock> parent;
  list<shared_ptr<Block>>::iterator partitn_loc;
  list<shared_ptr<Block>>::iterator parent_loc;
};  // struct Block

// This map keeps track of the 1 or 2 new blocks created during a split. After
// splitting is done, the map is iterated over, adding all of the newly created
// split blocks to the list.
using SplitBlockMap = map<shared_ptr<Block>, std::pair<shared_ptr<Block>,
                                                       shared_ptr<Block>>>;

// An 'SuperBlock' is a non-empty list of pointers to 'Blocks' refered to as
// its 'children'. Also keeps tracks of a map 'count' which records count(k, S).
// For a given node k, and a set S, count(k, S) is the number of edges going
// from k into the set of elements S. S in this case is the union of all the
// elements of the children of the SuperBlock.
struct SuperBlock {
 public:
  explicit SuperBlock() : children(), count() {}
  explicit SuperBlock(shared_ptr<Block> block) : children(1, block), count() {
    block->parent_loc = children.begin();
  }
  list<shared_ptr<Block>> children;
  map<NodeId, int> count;
};  // struct SuperBlock

// RefinementData keeps track of all of the main data structures used in
// partition refinement.
// 'blocks' is the partition we refine throughout, and eventually will output
// (properly formatted as a map).
// 'super_blocks' is a coarser version of 'blocks'.
// 'compound_blocks' is a list of 'compound' blocks of super_blocks, i.e.
// super blocks that contain multiple blocks. When compound_blocks is empty
// the algorithm terminates.
// Used for looking up which block a node is in.
struct RefinementData {
 public:
  list<shared_ptr<Block>> list_partition;
  list<shared_ptr<SuperBlock>> super_blocks;
  list<shared_ptr<SuperBlock>> compound_blocks;
  map<NodeId, shared_ptr<Block>> block_map;
};  // struct RefinementData

// A Splitter keeps track of the data structures used in splitting blocks.
// 'compound_splitter' is the compound_block used for performing a split.
// 'block_splitter' is the Block that is picked from 'compound_splitter'.
// 'block_splitter_parent' is the new SuperBlock that contains 'block_splitter'
// for bookkeeping purposes.
// 'block_splitter_preimage' is the preimage of 'block_splitter' and is
// precomputed to make splitting efficient.
struct Splitter {
 public:
  shared_ptr<SuperBlock> compound_splitter;
  shared_ptr<Block> block_splitter;
  shared_ptr<SuperBlock> block_splitter_parent;
  set<NodeId> block_splitter_preimage;
};  // struct Splitter

// Takes a list of Block's and outputs a partition in map<NodeId, int> form as
// required by graph::QuotientGraph.
map<NodeId, int> ConvertListToMapPartition(
    const list<shared_ptr<Block>>& list_partition) {
  map<NodeId, int> blocks_partition;
  int block_id = 0;
  for (auto& block : list_partition) {
    for (NodeId node : block->elements) {
      blocks_partition.insert({node, block_id});
    }
    ++block_id;
  }
  return blocks_partition;
}

// Adds a new block containing 'node' to the records in 'data'.
void AddNewBlock(const shared_ptr<SuperBlock>& whole_set,
                 RefinementData* data, NodeId node) {
  data->list_partition.emplace_back(std::make_shared<Block>(node));
  shared_ptr<Block> new_block(data->list_partition.back());
  new_block->partitn_loc = std::prev(data->list_partition.end());
  new_block->parent = whole_set;
  data->block_map.insert({node, new_block});
}

// Iterates through the partition and makes any necessary new blocks and marks
// any empty blocks.
void MakeNewAndEmpty(const LabeledGraph& graph,
                     RefinementData* data,
                     shared_ptr<SuperBlock> whole_set,
                     list<shared_ptr<Block>>* new_blocks,
                     list<shared_ptr<Block>>* empty_blocks) {
  for (auto& block : data->list_partition) {
    shared_ptr<Block> new_block = std::make_shared<Block>();
    for (NodeId node : block->elements) {
      if (graph.GetSuccessors(node).empty()) {
        new_block->elements.insert(node);
        data->block_map[node] = new_block;
      }
    }
    for (NodeId moved_node : new_block->elements) {
      block->elements.erase(moved_node);
    }
    if (new_block->elements.empty()) {
      continue;
    }
    new_block->parent = whole_set;
    whole_set->children.push_back(new_block);
    new_block->parent_loc = std::prev(whole_set->children.end());
    new_blocks->push_back(new_block);
    // If old block is now empty, mark it to be removed.
    if (block->elements.empty()) {
      empty_blocks->push_back(block);
    }
  }
}

// Splits the partition with respect to the whole set.
void SingleBlockPartitionSplit(const LabeledGraph& graph,
                               RefinementData* data) {
  auto whole_set = data->super_blocks.back();
  // List of new blocks when splitting.
  list<shared_ptr<Block>> new_blocks;
  // List of blocks that become empty when splitting.
  list<shared_ptr<Block>> empty_blocks;

  MakeNewAndEmpty(graph, data, whole_set, &new_blocks, &empty_blocks);

  // Cleanup, adding new blocks, removing empty, old blocks.
  for (const auto& new_block: new_blocks) {
    data->list_partition.push_back(new_block);
    new_block->partitn_loc = std::prev(data->list_partition.end());
  }
  for (const auto& block : empty_blocks) {
    data->list_partition.erase(block->partitn_loc);
    auto parent = block->parent.lock();
    parent->children.erase(block->parent_loc);
  }
}

// Intializes the data structures 'list_partition', 'super_blocks',
// 'compound_blocks', and block_map. Converts 'partition' from a map to a list
// and populates 'list_partition'. 'super_blocks' and 'compound_blocks' are
// initialized to a single element containing all of the blocks.
void InitializeDataStructures(const LabeledGraph& graph,
                              const map<NodeId, int>& partition,
                              RefinementData* data) {
  map<int, shared_ptr<Block>> initializationMap;
  data->super_blocks.emplace_back(std::make_shared<SuperBlock>());
  shared_ptr<SuperBlock> whole_set = data->super_blocks.back();
  for (auto& pair : partition) {
    NodeId node = pair.first;
    int block_id = pair.second;
    auto init_it = initializationMap.find(block_id);
    if (init_it == initializationMap.end()) {
      AddNewBlock(whole_set, data, node);
      initializationMap[block_id] = data->list_partition.back();
    } else {
      init_it->second->elements.insert(node);
      data->block_map.insert({node, init_it->second});
    }
    whole_set->count.insert({node, graph.GetSuccessors(node).size()});
  }
  for (auto& block : data->list_partition) {
    whole_set->children.push_back(block);
    block->parent_loc = std::prev(whole_set->children.end());
  }
  SingleBlockPartitionSplit(graph, data);
  if (whole_set->children.size() > 1) {
    data->compound_blocks.push_back(whole_set);
  }
}

// Picks a group of Blocks to use for splitting. Also picks one of blocks in
// that group for splitting.
void PickSplitter(RefinementData* data, Splitter* split) {
  split->compound_splitter = data->compound_blocks.front();
  data->compound_blocks.pop_front();
  auto compound_it = split->compound_splitter->children.begin();
  split->block_splitter = *compound_it;
  ++compound_it;
  if (split->block_splitter->elements.size() >
      (*compound_it)->elements.size()) {
    split->block_splitter = *compound_it;
    split->compound_splitter->children.erase(compound_it);
  } else {
    split->compound_splitter->children.pop_front();
  }
}

void UpdateCoarseCompound(RefinementData* data, Splitter* split) {
  data->super_blocks.emplace_back(
      std::make_shared<SuperBlock>(split->block_splitter));
  split->block_splitter_parent = data->super_blocks.back();
  split->block_splitter->parent = split->block_splitter_parent;
  if (split->compound_splitter->children.size() > 1) {
    data->compound_blocks.push_back(split->compound_splitter);
  }
}

void ComputePreimage(const LabeledGraph& graph,
                     Splitter* split) {
  for (NodeId node : split->block_splitter->elements) {
    set<NodeId> node_predecessors = graph.GetPredecessors(node);
    for (NodeId predecessor : node_predecessors) {
      split->block_splitter_preimage.insert(predecessor);
      auto count_it = split->block_splitter_parent->count.find(predecessor);
      if (count_it == split->block_splitter_parent->count.end()) {
        split->block_splitter_parent->count.insert({predecessor, 1});
      } else {
        count_it->second += 1;
      }
    }
  }
}

void InitializeSplitter(const LabeledGraph& graph,
                        RefinementData* data, Splitter* split) {
  UpdateCoarseCompound(data, split);
  ComputePreimage(graph, split);
}

void UpdateCounts(Splitter* split) {
  auto preimage_end_it = split->block_splitter_preimage.end();
  for (auto preimage_it = split->block_splitter_preimage.begin();
       preimage_it != preimage_end_it; ++preimage_it) {
    NodeId node = *preimage_it;
    auto count_it = split->compound_splitter->count.find(node);
    count_it->second -= split->block_splitter_parent->count.at(node);
    if (count_it->second < 1) {
      split->compound_splitter->count.erase(count_it);
    }
  }
}

// After splitting, cleanup and update data. Delete empty blocks, add new
// compound blocks to compound_blocks.
void CleanupSplit(const SplitBlockMap& split_blocks,
                  RefinementData* data) {
  auto split_end_it = split_blocks.end();
  for (auto split_it = split_blocks.begin(); split_it != split_end_it;
       ++split_it) {
    shared_ptr<Block> D = split_it->first;
    auto associated_pair = split_it->second;
    int num_new_blocks = 0;
    // Adding new blocks.
    if (associated_pair.first) {
      data->list_partition.push_back(associated_pair.first);
      associated_pair.first->partitn_loc =
          std::prev(data->list_partition.end());
      ++num_new_blocks;
    }
    if (associated_pair.second) {
      data->list_partition.push_back(associated_pair.second);
      associated_pair.second->partitn_loc =
          std::prev(data->list_partition.end());
      ++num_new_blocks;
    }
    // If the old block is now empty, remove it.
    auto parent = D->parent.lock();
    if (D->elements.empty()) {
      data->list_partition.erase(D->partitn_loc);
      parent->children.erase(D->parent_loc);
      --num_new_blocks;
    }
    if (num_new_blocks > 0 && parent->children.size() - num_new_blocks == 1) {
      data->compound_blocks.push_back(parent);
    }
  }
}

// Splits the blocks in 'refinement_data' into at most three parts.
void ThreeWaySplit(RefinementData* data,
                   Splitter* split) {
  // Go through the preimage and split the blocks. Keep track of which blocks
  // are split in 'split_blocks'.
  const auto& block_splitter_count = split->block_splitter_parent->count;
  const auto& compound_splitter_count = split->compound_splitter->count;
  SplitBlockMap split_blocks;
  for (NodeId node : split->block_splitter_preimage) {
    shared_ptr<Block> D = data->block_map.at(node);
    auto split_it = split_blocks.find(D);
    if (split_it == split_blocks.end()) {
      auto temp = split_blocks.emplace(
          std::make_pair(D, std::make_pair(nullptr, nullptr)));
      split_it = temp.first;
    }
    shared_ptr<Block>* associated_block;
    if (block_splitter_count.at(node) != compound_splitter_count.at(node)) {
      associated_block = &(split_it->second.first);
    } else {
      associated_block = &(split_it->second.second);
    }
    if (!(*associated_block)) {
      auto parent = D->parent.lock();
      parent->children.emplace_back(std::make_shared<Block>(node));
      *associated_block = parent->children.back();
      (*associated_block)->parent = D->parent;
      (*associated_block)->parent_loc =
          std::prev(parent->children.end());
    } else {
      (*associated_block)->elements.insert(node);
    }
    D->elements.erase(node);
    data->block_map.at(node) = (*associated_block);
  }
  CleanupSplit(split_blocks, data);
}

void SplitBlocks(RefinementData* data,
                 Splitter* split) {
  ThreeWaySplit(data, split);
  UpdateCounts(split);
}

}  // namespace

// Algorithm overiew:
// Data structures:
//  The algorithm maintains four data structures: 'list_partition',
//  'super_blocks', 'compound_blocks', and 'block_map'.
//
//  'list_partition' maintains a list of all of the blocks currently in our
//  refinement and can be logically though of as the 'current' refinement.
//
//  'super_blocks' maintains a list of groups of blocks.
//
//  'compound_blocks' is a subset of 'super_blocks'. A 'compound' block is a
//  SuperBlock that contains at least 2 Blocks. These blocks will be used to
//  refine the partition.
//
//  'block_map' keeps  track of which 'Block' each node belongs to at all times.
//
// The Paige-Tarjan algorithm for partition refinement initializes the data
// structures of the algorithm and then proceeds as follows:
// While compound_blocks is not empty:
//    Pick a Splitter from compound blocks
//    Do some pre-computation to speed up splitting
//    Split the blocks with the Splitter
//
// Once this is done, blocks will have the desired partition. We then convert
// this partition into a map<NodeId, int> and return it.
map<NodeId, int> RefinePartition(const LabeledGraph& graph,
                                 const map<NodeId, int>& partition) {
  RefinementData data;
  InitializeDataStructures(graph, partition, &data);
  while (!data.compound_blocks.empty()) {
    Splitter split;
    PickSplitter(&data, &split);
    InitializeSplitter(graph, &data, &split);
    SplitBlocks(&data, &split);
  }
  return ConvertListToMapPartition(data.list_partition);
}

}  // namespace graph_analyzer

}  // namespace tervuren
