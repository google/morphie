#include "third_party/logle/account_access_analyzer.h"

#include <sstream>

#include "third_party/logle/gtest.h"
#include "third_party/logle/util/csv.h"
#include "third_party/logle/util/status.h"
#include "third_party/logle/util/string_utils.h"

namespace third_party_logle {
namespace {

void TestCSVInitialization(const char* kInput, Code code) {
  std::unique_ptr<util::CSVParser> parser(
      new util::CSVParser(new std::stringstream(kInput)));
  AccessAnalyzer access_analyzer;
  util::Status s = access_analyzer.Initialize(std::move(parser));
  EXPECT_EQ(code, s.code());
}

void TestInvalidCSVInitialization(const char* kInput) {
  TestCSVInitialization(kInput, Code::INVALID_ARGUMENT);
}

void TestValidCSVInitialization(const char* kInput) {
  TestCSVInitialization(kInput, Code::OK);
}

// The next few tests below check that the analyzer cannot be initialized with
// invalid input.
TEST(AccessAnalyzerTest, RejectsEmptyCSVInput) {
  TestInvalidCSVInitialization("");
  TestInvalidCSVInitialization("\n\n");
}

TEST(AccessAnalyzerTest, RejectsNamelessCSVFields) {
  TestInvalidCSVInitialization(",fromx,tox\n");
  TestInvalidCSVInitialization("fromx,tox,\n");
  TestInvalidCSVInitialization("fromx,,tox\n");
}

TEST(AccessAnalyzerTest, RejectsDuplicateCSVFieldNames) {
  TestInvalidCSVInitialization("fromx,fromx,tox\n");
  TestInvalidCSVInitialization("fromx,tox,fromx\n");
}

TEST(AccessAnalyzerTest, RejectsHeaderOnlyInput) {
  TestInvalidCSVInitialization("fromx,attr_actor_title,tox,attr_count");
}

const char header[] =
    "fromx,tox,attr_actor_manager,attr_actor_cost_center,"
    "attr_first_access,"
    "sttr_last_access,attr_count,attr_actor_title";

TEST(AccessAnalyzerTest, AcceptsNonEmptyOnlyInput) {
  string content = "\nabc@xyz.tuv,def@tuv.xyz,Alpha,None,1,2,3,Engineer";
  TestValidCSVInitialization(util::StrCat(header, content).c_str());
}

// Test that running the analyzer on 'kInput' produces a graph with the expected
// number of nodes and edges.
void TestGraphConstruction(const char* kInput, int num_nodes, int num_edges) {
  std::unique_ptr<util::CSVParser> parser(
      new util::CSVParser(new std::stringstream(kInput)));
  AccessAnalyzer access_analyzer;
  util::Status s = access_analyzer.Initialize(std::move(parser));
  ASSERT_TRUE(s.ok());
  s = access_analyzer.BuildAccessGraph();
  EXPECT_TRUE(s.ok()) << "Graph construction failed.";
  EXPECT_EQ(num_nodes, access_analyzer.NumGraphNodes())
      << "Error when processing :\n"
      << kInput;
  EXPECT_EQ(num_edges, access_analyzer.NumGraphEdges())
      << "Error when processing :\n"
      << kInput;
}

TEST(AccessAnalyzerTest, TestGraphConstruction) {
  string content1 = "\nabc@xyz.tuv,def@tuv.xyz,Alpha,None,1,2,3,Engineer";
  // One actor and one account and one edge between them.
  TestGraphConstruction(util::StrCat(header, content1).c_str(), 2, 1);
  // Duplicating the data will not increase the number of edges in the graph.
  TestGraphConstruction(util::StrCat(header, content1, content1).c_str(), 2, 1);
  string content2 = "\nabc@xyz.tuv,ghi@tuv.xyz,Alpha,None,1,2,3,Engineer";
  TestGraphConstruction(util::StrCat(header, content1, content2).c_str(), 3, 2);
  string content3 = "\ndef@xperson,ghi@tuv.xyz,Alpha,None,1,2,3,Engineer";
  TestGraphConstruction(util::StrCat(header, content1, content2).c_str(), 3, 2);
}

}  // namespace
}  // namespace third_party_logle
