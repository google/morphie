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

#include "dot_printer.h"

#include <algorithm>
#include <boost/regex.hpp>
#include <set>
#include "base/vector.h"

#include "ast.h"
#include "ast.pb.h"
#include "base/string.h"
#include "base/string.h"
#include "gtest.h"
#include "labeled_graph.h"
#include "plaso_event.h"
#include "type.h"
#include "type_checker.h"
#include "util/status.h"
#include "util/string_utils.h"
#include "value.h"

namespace tervuren {
namespace {

using ast::type::Types;

// These tests to not ensure that the generated strings conform to the GraphViz
// DOT grammar but provide a rough approximation of such checks. The flag (?s)
// in the expressions below enables '.' to match newlines.

// Tags in the DOT language.
const char kTableRegEx[] = R"(<table[^>]*>|</table>)";
const char kRowRegEx[] = R"(<tr[^>]*>|</tr>)";
const char kCellRegEx[] = R"(<td[^>]*>|</td>)";
const char kFontRegEx[] = R"(<font[^>]*>|</font>)";
const char kBreakRegEx[] = R"(<br[^>]*/>)";
const char kTextStyleRegEx[] = R"(<b>|</b>|<u>|</u>|<i>|</i>)";
const char kTagRegEx[] = R"((?s)[^<]*(<[^>]*>))";
const char kTaggedLabelRegEx[] = R"((?s).*\[.*label\s*=\s*<(.*)>\])";

// Returns true if 's' is a valid DOT tag.
bool IsDotTag(const string& s) {
  return boost::regex_match(s, boost::regex{kTableRegEx}) ||
         boost::regex_match(s, boost::regex{kRowRegEx}) ||
         boost::regex_match(s, boost::regex{kCellRegEx}) ||
         boost::regex_match(s, boost::regex{kFontRegEx}) ||
         boost::regex_match(s, boost::regex{kBreakRegEx}) ||
         boost::regex_match(s, boost::regex{kTextStyleRegEx});
}

// Returns true if 's' is enclosed by square brackets and contains a quoted
// label.
bool IsLabeledAttribute(const string& s) {
  return boost::regex_match(s, boost::regex(R"(^\[.*label=".*"\]$)"));
}

// Returns true if 's' is enclosed by square brackets and contains a label.
bool IsTagLabeledAttribute(const string& s) {
  return boost::regex_match(s, boost::regex(R"((?s)^\[.*label=<.*>\]$)"));
}

// Returns true if 's' has the form
//  <whitespace><identifier><whitespace>[attribute];
bool IsNodeDeclaration(const string& s) {
  string node_ex(R"((?s)^\s*\S*\s*\[.*\];$)");
  boost::regex node_regex(node_ex);
  return boost::regex_match(s, node_regex);
}

bool IsEdgeDeclaration(const string& s) {
  return boost::regex_match(
      s, boost::regex(R"((?s)^\s*\S*\s*->\s*\S*\s*\[.*\];$)"));
}

bool HasGraphDeclaration(const string& s) {
  return boost::regex_match(s, boost::regex(R"((?s)^digraph\s*\S*\s*\{.*\}$)"));
}

// Returns the set of tags in 's'.
vector<string> GetTags(const string& s) {
  vector<string> tags;
  boost::regex label_regex(kTaggedLabelRegEx);
  // There are two subgroups in the regular expression kTaggedLabelRegEx. The
  // first group matches the name and the second matches the contents of a label
  // bounded by the tokens '<' and '>'. The code below specifies that the boost
  // regex iterator should iterate over the first and second submatches.
  int const sub_matches[] = {1, 2};
  boost::sregex_token_iterator label_begin(s.begin(), s.end(), label_regex,
                                           sub_matches);
  boost::sregex_token_iterator label_end;
  for (auto label_it = label_begin; label_it != label_end; ++label_it) {
    boost::regex tag_regex(kTagRegEx);
    boost::sregex_token_iterator tag_begin(label_it->begin(), label_it->end(),
                                           tag_regex);
    boost::sregex_token_iterator tag_end;
    for (auto tag_it = tag_begin; tag_it != tag_end; ++tag_it) {
      tags.push_back(*tag_it);
    }
  }
  return tags;
}

AST MakeFilename(const string& filename) {
  return plaso::ToAST(plaso::ParseFilename(filename));
}

class LabeledGraphVisualizerTest : public ::testing::Test {
 protected:
  void AddNode(const string& tag, AST ast) {
    tast_.set_tag(tag);
    *tast_.mutable_ast() = ast;
    NodeId node_id = graph_.FindOrAddNode(tast_);
    label_strs_.emplace_back(dot_printer_.DotNode(node_id, tast_));
  }

  void AddEdge(NodeId source, NodeId target, const string& tag, AST ast) {
    tast_.set_tag(tag);
    *tast_.mutable_ast() = ast;
    graph_.FindOrAddEdge(source, target, tast_);
    label_strs_.emplace_back(dot_printer_.DotEdge(source, target, tast_));
  }

  string attr_;
  string dot_str_;
  AST label_ast_;
  DotPrinter dot_printer_;
  TaggedAST tast_;
  LabeledGraph graph_;
  vector<string> label_strs_;
};

// These tests check the formatting of attributes and declarations and also
// check that if a declaration is generated directly, it will contain the
// same attribute as when the attribute is generated explicitly.
TEST_F(LabeledGraphVisualizerTest, FormattingEmptyFilename) {
  label_ast_ = MakeFilename("");
  attr_ = DotPrinter::FileAttribute(label_ast_);
  EXPECT_TRUE(IsLabeledAttribute(attr_)) << "Malformed label:" << attr_;
  tast_.set_tag(ast::kFileTag);
  *tast_.mutable_ast() = label_ast_;
  dot_str_ = dot_printer_.DotNode(0, tast_);
  EXPECT_TRUE(IsNodeDeclaration(dot_str_)) << "Malformed node :" << dot_str_;
  EXPECT_NE(dot_str_.find(attr_), string::npos)
      << "The node:" << dot_str_ << " is missing the label:" << attr_;
}

TEST_F(LabeledGraphVisualizerTest, FormattingNonEmptyFilename) {
  label_ast_ = MakeFilename("/an/example/filename.txt");
  attr_ = DotPrinter::FileAttribute(label_ast_);
  EXPECT_TRUE(IsLabeledAttribute(attr_)) << "Malformed label:" << attr_;
  tast_.set_tag(ast::kFileTag);
  *tast_.mutable_ast() = label_ast_;
  dot_str_ = dot_printer_.DotNode(0, tast_);
  EXPECT_TRUE(IsNodeDeclaration(dot_str_)) << "Malformed node :" << dot_str_;
  EXPECT_NE(dot_str_.find(attr_), string::npos)
      << "The node:" << dot_str_ << " is missing the label:" << attr_;
}

TEST_F(LabeledGraphVisualizerTest, FormattingEmptyIPAddress) {
  label_ast_ = ast::value::MakeString("");
  attr_ = DotPrinter::IPAddressAttribute(label_ast_);
  EXPECT_TRUE(IsLabeledAttribute(attr_)) << "Malformed label:" << attr_;
  tast_.set_tag(ast::kIPAddressTag);
  *tast_.mutable_ast() = label_ast_;
  dot_str_ = dot_printer_.DotNode(0, tast_);
  EXPECT_NE(dot_str_.find(attr_), string::npos)
      << "The node:" << dot_str_ << " is missing the label:" << attr_;
}

TEST_F(LabeledGraphVisualizerTest, FormattingNonEmptyIPAddress) {
  label_ast_ = ast::value::MakeString("10.0.0.1");
  attr_ = DotPrinter::IPAddressAttribute(label_ast_);
  EXPECT_TRUE(IsLabeledAttribute(attr_)) << "Malformed label:" << attr_;
  tast_.set_tag(ast::kIPAddressTag);
  *tast_.mutable_ast() = label_ast_;
  dot_str_ = dot_printer_.DotNode(0, tast_);
  EXPECT_NE(dot_str_.find(attr_), string::npos)
      << "The node:" << dot_str_ << " is missing the label:" << attr_;
}

TEST_F(LabeledGraphVisualizerTest, FormattingNonEmptyURL) {
  label_ast_ = ast::value::MakeString("www.example-url.net");
  attr_ = DotPrinter::URLAttribute(label_ast_);
  EXPECT_TRUE(IsLabeledAttribute(attr_)) << "Malformed label:" << attr_;
  tast_.set_tag(ast::kURLTag);
  *tast_.mutable_ast() = label_ast_;
  dot_str_ = dot_printer_.DotNode(0, tast_);
  EXPECT_TRUE(IsNodeDeclaration(dot_str_)) << "Malformed node :" << dot_str_;
  EXPECT_NE(dot_str_.find(attr_), string::npos)
      << "The node:" << dot_str_ << " is missing the label:" << attr_;
}

TEST_F(LabeledGraphVisualizerTest, FormattingEmptyURL) {
  label_ast_ = ast::value::MakeString("");
  attr_ = DotPrinter::URLAttribute(label_ast_);
  EXPECT_TRUE(IsLabeledAttribute(attr_)) << "Malformed label:" << attr_;
  tast_.set_tag(ast::kURLTag);
  *tast_.mutable_ast() = label_ast_;
  dot_str_ = dot_printer_.DotNode(0, tast_);
  EXPECT_TRUE(IsNodeDeclaration(dot_str_)) << "Malformed node :" << dot_str_;
  EXPECT_NE(dot_str_.find(attr_), string::npos)
      << "The node:" << dot_str_ << " is missing the label:" << attr_;
}

const char kRandomTag_[] = "RandomTag";
const char kEdgeTag_[] = "EdgeTag";

TEST_F(LabeledGraphVisualizerTest, FormattingEmptyStringNodeTag) {
  label_ast_ = ast::value::MakeString("");
  attr_ = DotPrinter::NodeAttribute(kRandomTag_, label_ast_);
  EXPECT_TRUE(IsTagLabeledAttribute(attr_)) << "Malformed label:" << attr_;
  tast_.set_tag(kRandomTag_);
  *tast_.mutable_ast() = label_ast_;
  dot_str_ = dot_printer_.DotNode(0, tast_);
  EXPECT_TRUE(IsNodeDeclaration(dot_str_)) << "Malformed node :" << dot_str_;
  EXPECT_NE(dot_str_.find(attr_), string::npos)
      << "The node:" << dot_str_ << " is missing the label:" << attr_;
}

TEST_F(LabeledGraphVisualizerTest, FormattingNonEmptyStringNodeTag) {
  label_ast_ = ast::value::MakeString("Random Text");
  attr_ = DotPrinter::NodeAttribute(kRandomTag_, label_ast_);
  EXPECT_TRUE(IsTagLabeledAttribute(attr_)) << "Malformed label:" << attr_;
  tast_.set_tag(kRandomTag_);
  *tast_.mutable_ast() = label_ast_;
  dot_str_ = dot_printer_.DotNode(0, tast_);
  EXPECT_TRUE(IsNodeDeclaration(dot_str_)) << "Malformed node :" << dot_str_;
  EXPECT_NE(dot_str_.find(attr_), string::npos)
      << "The node:" << dot_str_ << " is missing the label:" << attr_;
}

TEST_F(LabeledGraphVisualizerTest, EscapingStringLabel) {
  label_ast_ = ast::value::MakeString("<fake tag&>");
  attr_ = DotPrinter::NodeAttribute(kRandomTag_, label_ast_);
  int num_tags = GetTags(attr_).size();
  EXPECT_EQ(0, num_tags) << "Expected 0 tags but found " << num_tags << " in "
                         << attr_;
  // The first two comparisons below have '+1' to account for the delimiter.
  EXPECT_EQ(num_tags + 1, std::count(attr_.begin(), attr_.end(), '<'))
      << "Expected " << num_tags << " '<' symbols in " << attr_;
  EXPECT_EQ(num_tags + 1, std::count(attr_.begin(), attr_.end(), '>'))
      << "Expected " << num_tags << " '>' symbols in " << attr_;
  // There is one ampersand for each escaped symbol.
  EXPECT_EQ(3, std::count(attr_.begin(), attr_.end(), '&'))
      << "Expected 3 '&' symbols in :" << attr_;
  EXPECT_NE(string::npos, attr_.find("&lt"))
      << "Expected '<' to be escaped as '&lt' in :" << attr_;
  EXPECT_NE(string::npos, attr_.find("&gt"))
      << "Expected '>' to be escaped as '&gt' in :" << attr_;
  EXPECT_NE(string::npos, attr_.find("&amp"))
      << "Expected '&' to be escaped as '&amp' in :" << attr_;
}

TEST_F(LabeledGraphVisualizerTest, FormattingTupleNodeTag) {
  // Make a pair consisting of a bool and a string.
  AST tuple = ast::type::MakeTuple(
      "pair", true,
      {ast::type::MakeBool("one", true), ast::type::MakeInt("two", true)});
  label_ast_ = ast::value::MakeNullTuple(2);
  ast::value::SetField(tuple, 0, ast::value::MakeBool(true), &label_ast_);
  ast::value::SetField(tuple, 1, ast::value::MakeInt(-3), &label_ast_);
  // Check tags and escape sequences.
  attr_ = DotPrinter::NodeAttribute(kRandomTag_, label_ast_);
  EXPECT_TRUE(IsTagLabeledAttribute(attr_)) << "Malformed label:" << attr_;
  int num_tags = GetTags(attr_).size() + 1;
  // The comparison increments the number of tags because a label is enclosed in
  // '<' and '>' delimiters.
  EXPECT_EQ(num_tags, std::count(attr_.begin(), attr_.end(), '<'))
      << "Expected " << num_tags << " '<' symbols in " << attr_;
  EXPECT_EQ(num_tags, std::count(attr_.begin(), attr_.end(), '>'))
      << "Expected " << num_tags << " '<' symbols in " << attr_;

  tast_.set_tag(kRandomTag_);
  *tast_.mutable_ast() = label_ast_;
  dot_str_ = dot_printer_.DotNode(0, tast_);
  EXPECT_TRUE(IsNodeDeclaration(dot_str_)) << "Malformed node :" << dot_str_;
  EXPECT_NE(dot_str_.find(attr_), string::npos)
      << "The node:" << dot_str_ << " is missing the label:" << attr_;
}

TEST_F(LabeledGraphVisualizerTest, FormattingEmptyStringEdgeTag) {
  label_ast_ = ast::value::MakeString("");
  attr_ = DotPrinter::EdgeAttribute(kRandomTag_, label_ast_);
  EXPECT_TRUE(IsTagLabeledAttribute(attr_)) << "Malformed label:" << attr_;
  tast_.set_tag(kRandomTag_);
  *tast_.mutable_ast() = label_ast_;
  dot_str_ = dot_printer_.DotEdge(0, 1, tast_);
  EXPECT_TRUE(IsEdgeDeclaration(dot_str_)) << "Malformed edge:" << dot_str_;
  EXPECT_NE(dot_str_.find(attr_), string::npos)
      << "The node:" << dot_str_ << " is missing the label:" << attr_;
}

TEST_F(LabeledGraphVisualizerTest, FormattingNonEmptyStringEdgeTag) {
  label_ast_ = ast::value::MakeString("Random Text");
  attr_ = DotPrinter::EdgeAttribute(kRandomTag_, label_ast_);
  EXPECT_TRUE(IsTagLabeledAttribute(attr_)) << "Malformed label:" << attr_;
  tast_.set_tag(kRandomTag_);
  *tast_.mutable_ast() = label_ast_;
  dot_str_ = dot_printer_.DotEdge(0, 1, tast_);
  EXPECT_TRUE(IsEdgeDeclaration(dot_str_)) << "Malformed edge:" << dot_str_;
  EXPECT_NE(dot_str_.find(attr_), string::npos)
      << "The node:" << dot_str_ << " is missing the label:" << attr_;
}

// Create a graph with all the supported tag types and one unsupported tag type.
util::Status Initialize(LabeledGraph* graph) {
  Types node_types;
  node_types.emplace(ast::kFileTag, ast::type::MakeFile());
  node_types.emplace(ast::kIPAddressTag, ast::type::MakeIPAddress());
  node_types.emplace(ast::kURLTag, ast::type::MakeURL());
  node_types.emplace(kRandomTag_, ast::type::MakeString(kRandomTag_, false));
  set<string> node_tags = {ast::kFileTag, ast::kIPAddressTag,
                           ast::kURLTag};
  Types edge_types;
  edge_types.emplace(ast::kPrecedesTag, ast::type::MakeBool("", true));
  edge_types.emplace(kEdgeTag_, ast::type::MakeString(kEdgeTag_, false));
  set<string> edge_tags = {ast::kPrecedesTag};
  AST graph_type = ast::type::MakeString("System", false);
  return graph->Initialize(node_types, node_tags, edge_types, edge_tags,
                           graph_type);
}

// This test checks that the headers are appropriate, but does not check that
// the DOT graph represents the empty graph.
TEST_F(LabeledGraphVisualizerTest, DotEmptyGraph) {
  EXPECT_TRUE(Initialize(&graph_).ok());
  dot_str_ = dot_printer_.DotGraph(graph_);
  EXPECT_TRUE(HasGraphDeclaration(dot_str_)) << "Malformed graph:" << dot_str_;
}

TEST_F(LabeledGraphVisualizerTest, UnconnectedNodes) {
  EXPECT_TRUE(Initialize(&graph_).ok());
  AddNode(ast::kFileTag, MakeFilename("/example/of/a/file.txt"));
  AddNode(ast::kIPAddressTag, ast::value::MakeString("10.0.0.1"));
  AddNode(ast::kURLTag, ast::value::MakeString("www.example-url.net"));
  AddNode(kRandomTag_, ast::value::MakeString(kRandomTag_));

  dot_str_ = dot_printer_.DotGraph(graph_);
  EXPECT_TRUE(HasGraphDeclaration(dot_str_)) << "Malformed graph:" << dot_str_;
  for (const auto& node_str : label_strs_) {
    EXPECT_NE(dot_str_.find(node_str), string::npos)
      << "The graph:" << dot_str_ << " is missing the node:" << node_str;
  }
}

TEST_F(LabeledGraphVisualizerTest, CompleteGraphDefaultAttributes) {
  EXPECT_TRUE(Initialize(&graph_).ok());
  AddNode(ast::kFileTag, MakeFilename("/example/of/a/file.txt"));
  AddNode(ast::kURLTag, ast::value::MakeString("www.example-url.net"));
  AddNode(kRandomTag_, ast::value::MakeString(kRandomTag_));
  label_strs_.clear();
  AddEdge(2, 0, kEdgeTag_, ast::value::MakeString("Edge 1"));
  AddEdge(2, 1, kEdgeTag_, ast::value::MakeString("Edge 2"));
  AddEdge(0, 2, ast::kPrecedesTag, ast::value::MakeBool(true));
  AddEdge(0, 1, ast::kPrecedesTag, ast::value::MakeBool(true));
  AddEdge(2, 2, kEdgeTag_, ast::value::MakeString("Self-Loop"));

  dot_str_ = dot_printer_.DotGraph(graph_);
  EXPECT_TRUE(HasGraphDeclaration(dot_str_)) << "Malformed graph:" << dot_str_;
  for (const auto& edge_str : label_strs_) {
    EXPECT_NE(dot_str_.find(edge_str), string::npos)
      << "The graph:" << dot_str_ << " is missing the edge:" << edge_str;
  }
}

}  // namespace
}  // namespace tervuren
