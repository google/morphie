// A Curio stream definition determines how data will be processed to generate a
// stream (which is a Dremel table). A stream 't' depends on a stream 's' if the
// definition of 't' uses the stream 's'. Dependence is determined by stream
// definitions and need not be transitive. For example, if 't' is defined in
// terms of 's' and 's' is defined in terms of 'r', and the definition of 't'
// does not explicitly involve 'r', then 't' does not depend on 'r'.
//
// A stream dependency graph represents dependencies between stream definitions.
// If 't' depends on 's', then 's' is a producer and 't' is a consumer of data.
#ifndef THIRD_PARTY_LOGLE_STREAM_DEPENDENCY_GRAPH_H_
#define THIRD_PARTY_LOGLE_STREAM_DEPENDENCY_GRAPH_H_

#include "third_party/logle/base/string.h"
#include "third_party/logle/graph_interface.h"
#include "third_party/logle/labeled_graph.h"
#include "third_party/logle/util/status.h"

namespace third_party_logle {

// The StreamDependencyGraph class implements a stream dependency graph. The
// nodes in the graph are labels represented as abstract syntax trees (see
// ast.proto). Each node label is a pair of strings with the first string
// representing the stream identifier and the second string representing the
// stream name. There are no edge labels.
//
// The function Initialize() must be called before other functions are called.
// The functions will crash otherwise.
class StreamDependencyGraph : public GraphInterface {
 public:
  StreamDependencyGraph() : is_initialized_(false) {}

  // Initialize the graph. Returns
  // - Status::OK - if a graph with the appropriate node and edge types has
  //   been created.
  // - Status::INTERNAL - otherwise, with the reason accessible via the
  //   Status::error_message() function of the Status object.
  util::Status Initialize();

  int NumNodes() const;
  int NumEdges() const;

  // Adds an edge for a dependency of consumer with id 'consumer_id' and name
  // 'consumer_name' on producer with id 'producer_id' and name 'producer_name'.
  // Creates nodes for consumer and producer if they do not already exist.
  void AddDependency(const string& consumer_id, const string& consumer_name,
                     const string& producer_id, const string& producer_name);

  // Return a representation of the graph in Graphviz DOT format.
  string ToDot() const;

 private:
  // This variable is set to false by the constructor and is set to 'true' if
  // Initialize() completes successfully. It is not modified thereafter.
  bool is_initialized_;

  LabeledGraph graph_;
};

}  // namespace third_party_logle
#endif  // THIRD_PARTY_LOGLE_STREAM_DEPENDENCY_GRAPH_H_
