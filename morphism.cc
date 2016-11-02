#include "morphism.h"

#include "util/map_utils.h"
#include "util/status.h"

namespace morphie {
namespace graph {

std::unique_ptr<LabeledGraph> Morphism::TakeOutput() {
  node_map_.clear();
  node_preimage_.clear();
  return std::move(output_graph_);
}

void Morphism::CopyInputType() {
  output_graph_.reset(new LabeledGraph());
  util::Status status = output_graph_->Initialize(
      input_graph_.GetNodeTypes(), input_graph_.GetUniqueNodeTags(),
      input_graph_.GetEdgeTypes(), input_graph_.GetUniqueEdgeTags(),
      input_graph_.GetGraphType());
  if (!status.ok()) {
    output_graph_.reset(nullptr);
  }
}

NodeId Morphism::FindOrCopyNode(NodeId input_node) {
  TaggedAST label = input_graph_.GetNodeLabel(input_node);
  return FindOrMapNode(input_node, label);
}

NodeId Morphism::FindOrMapNode(NodeId input_node, TaggedAST label) {
  auto map_it = node_map_.find(input_node);
  if (map_it != node_map_.end()) {
    return map_it->second;
  }
  NodeId output_node = output_graph_->FindOrAddNode(label);
  node_map_.insert({input_node, output_node});
  auto preimage_it = node_preimage_.find(output_node);
  if (preimage_it == node_preimage_.end()) {
    node_preimage_.insert({output_node, {input_node}});
  } else {
    preimage_it->second.insert(input_node);
  }
  return output_node;
}

EdgeId Morphism::FindOrCopyEdge(EdgeId input_edge) {
  TaggedAST label = input_graph_.GetEdgeLabel(input_edge);
  return FindOrMapEdge(input_edge, label);
}

EdgeId Morphism::FindOrMapEdge(EdgeId input_edge, TaggedAST label) {
  NodeId src = FindOrCopyNode(input_graph_.Source(input_edge));
  NodeId tgt = FindOrCopyNode(input_graph_.Target(input_edge));
  EdgeId output_edge = output_graph_->FindOrAddEdge(src, tgt, label);
  return output_edge;
}

util::Status Morphism::ComposeWith(Morphism* morphism) {
  if (output_graph_.get() != &morphism->input_graph_) {
    return util::Status(Code::INVALID_ARGUMENT,
                        "Trying to compose incompatible morphisms.");
  }
  node_map_ = util::Compose(node_map_, morphism->node_map_);
  node_preimage_ = util::Preimage(node_map_);
  output_graph_ = morphism->TakeOutput();
  return util::Status::OK;
}

}  // namespace graph
}  // namespace morphie
