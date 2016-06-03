#include "curio_analyzer.h"

#include <algorithm>

#include "util/logging.h"
#include "util/status.h"
#include "util/string_utils.h"

namespace {

// The maximum number of malformed JSON objects allowed in the input.
const int kMaxMalformedObjects = 1000000;

// The clock is a special stream that is guaranteed to be present.
const char kClockId[] = "[:clock]";
const char kNullDocErr[] = "The pointer to the document is null.";

// Check if the dependency tree has the required JSON fields.
bool HasRequiredFields(const Json::Value& json_tree) {
  return json_tree.isMember("Node") && json_tree["Node"].isMember("ID") &&
         json_tree["Node"]["ID"].isMember("Name") &&
         json_tree.isMember("Children");
}

}  // namespace

namespace logle {

util::Status CurioAnalyzer::Initialize(
    std::unique_ptr<::Json::Value> json_doc) {
  if (json_doc == nullptr) {
    return util::Status(Code::INVALID_ARGUMENT, kNullDocErr);
  }
  json_doc_ = std::move(json_doc);
  if (json_doc_->empty()) {
    return util::Status(Code::INVALID_ARGUMENT,
                        "The input document must not be empty.");
  }
  if (!json_doc_->isObject()) {
    return util::Status(Code::INVALID_ARGUMENT,
                        "The document must be an object.");
  }
  num_streams_processed_ = 0;
  num_streams_skipped_ = 0;
  return util::Status::OK;
}

util::Status CurioAnalyzer::BuildDependencyGraph() {
  CHECK(json_doc_ != nullptr, kNullDocErr);
  dependency_graph_.reset(new StreamDependencyGraph);
  util::Status status = dependency_graph_->Initialize();
  if (!status.ok()) {
    return status;
  }
  for (const auto& consumer_id : json_doc_->getMemberNames()) {
    // Every stream depends on the clock so the clock stream is not added to the
    // dependency graph to reduce structural and visual noise.
    if (consumer_id == kClockId) {
      continue;
    }
    if (!HasRequiredFields((*json_doc_)[consumer_id])) {
      status = IncrementSkipCounter();
      if (!status.ok()) {
        return status;
      }
      continue;
    }
    status = AddDependencies(consumer_id, (*json_doc_)[consumer_id]);
    ++num_streams_processed_;
    if (!status.ok()) {
      return status;
    }
  }
  return util::Status::OK;
}

int CurioAnalyzer::NumGraphNodes() const {
  return dependency_graph_ == nullptr ? 0 : dependency_graph_->NumNodes();
}

int CurioAnalyzer::NumGraphEdges() const {
  return dependency_graph_ == nullptr ? 0 : dependency_graph_->NumEdges();
}

// Recursively traverse the dependency tree rooted at 'consumer_tree'. The
// traversal skips a subtree below a node if that node is not well defined.
util::Status CurioAnalyzer::AddDependencies(const string& consumer_id,
                                            const Json::Value& consumer_tree) {
  util::Status status = util::Status::OK;
  string consumer_name = consumer_tree["Node"]["ID"]["Name"].asString();
  Json::Value producer_tree;
  for (const auto& producer_id : consumer_tree["Children"].getMemberNames()) {
    // Every stream depends on the clock the clock stream is not added to the
    // dependency graph to reduce structural and visual noise.
    if (producer_id == kClockId) {
      continue;
    }
    producer_tree = consumer_tree["Children"][producer_id];
    if (!HasRequiredFields(producer_tree)) {
      status = IncrementSkipCounter();
      if (!status.ok()) {
        return status;
      }
      continue;
    }
    string producer_name = producer_tree["Node"]["ID"]["Name"].asString();
    dependency_graph_->AddDependency(consumer_id, consumer_name, producer_id,
                                     producer_name);
    status = AddDependencies(producer_id, producer_tree);
    ++num_streams_processed_;
    if (!status.ok()) {
      return status;
    }
  }
  return status;
}

string CurioAnalyzer::DependencyGraphAsDot() const {
  return dependency_graph_ == nullptr ? "" : dependency_graph_->ToDot();
}

util::Status CurioAnalyzer::IncrementSkipCounter() {
  ++num_streams_skipped_;
  if (num_streams_skipped_ >= kMaxMalformedObjects) {
    return util::Status(
        Code::INVALID_ARGUMENT,
        util::StrCat("Over  ", std::to_string(kMaxMalformedObjects),
                     " malformed JSON objects in input. Aborting."));
  }
  return util::Status::OK;
}

}  // namespace logle
