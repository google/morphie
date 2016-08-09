#include "morphism.h"

#include "util/status.h"

namespace tervuren {
namespace graph {

void Morphism::CloneInputType() {
  output_graph_.reset(new LabeledGraph());
  util::Status status = output_graph_->Initialize(
      input_graph_.GetNodeTypes(), input_graph_.GetUniqueNodeTags(),
      input_graph_.GetEdgeTypes(), input_graph_.GetUniqueEdgeTags(),
      input_graph_.GetGraphType());
  if (!status.ok()) {
    output_graph_.reset(nullptr);
  }
}

NodeId Morphism::FindOrMapNode(NodeId input_node, TaggedAST label) {
  auto map_it = node_map_.find(input_node);
  if (map_it != node_map_.end()) {
    return map_it->second;
  }
  NodeId output_node = output_graph_->FindOrAddNode(label);
  node_map_.insert({input_node, output_node});
  return output_node;
}

}  // namespace graph
}  // namespace tervuren
