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

#include "graph/dot_printer.h"

#include <boost/algorithm/string/join.hpp>  // NOLINT
#include <boost/algorithm/string/replace.hpp>  // NOLINT
#include <boost/graph/directed_graph.hpp>  // NOLINT

#include "graph/ast.h"
#include "graph/type.h"
#include "graph/type_checker.h"
#include "graph/value.h"
#include "graph/value_checker.h"
#include "util/logging.h"
#include "util/status.h"
#include "util/string_utils.h"

namespace morphie {

namespace {

const char kTableHeader[] =
    R"(<table border="0"  cellborder="0" )"
    R"(cellpadding="1" bgcolor="#F8F8F8">)";

// The style strings control the way a node or an edge is rendered.
const char kFilenameStyle[] =
    "shape=folder,style=filled,fillcolor=wheat,fontname=Courier,fontsize=10";
const char kRemoteAddressStyle[] =
    "shape=component,style=filled,"
    "fillcolor=lightsteelblue,fontname=Arial,fontsize=10";
const char kRoundedBoxStyle[] =
    R"(shape=box,style="rounded,filled",fillcolor="#F8F8F8")";

const char kSolidBlackEdge[] =
    "penwidth=.5,arrowsize=.5,arrowhead=onormal,color=black";
const char kDashedBlackEdge[] =
    "penwidth=.5,arrowsize=.5,arrowhead=onormal,color=black,style=dashed";
const char kSolidGrayEdge[] =
    "penwidth=.5,arrowsize=.5,arrowhead=onormal,color=gray";
const char kDashedGrayEdge[] =
    "penwidth=.5,arrowsize=.5,arrowhead=onormal,color=gray,style=dashed";
const char kPrecedesStyle[] = "style=invis";

// Retrieve the string at position 'pos' in the list of strings represented by
// 'ast'. If 'ast' is not a list of strings, the method GetString will crash.
string GetListElement(const AST& ast, int pos) {
  CHECK(pos < ast.c_ast().arg_size(), "");
  return ast::value::GetString(ast.c_ast().arg(pos));
}

// Replace '<','>' and '&' with their corresponding escape sequences. The '&'
// symbol must be replaced first to avoid '&lt;' being replaced by '&amp;lt;'.
void AddEscapes(string* s) {
  boost::replace_all(*s, "&", "&amp;");
  boost::replace_all(*s, "<", "&lt;");
  boost::replace_all(*s, ">", "&gt;");
}

// Returns a string of the form "[shape=Box, label="foo"]" that defines the
// label and style attributes of a GraphViz node. If 'is_html_like' is true, the
// string will be of the form "[shape=Box, label=<foo>]", where the label
// delimiter is angluar brackets.
string JoinAttributes(const string& style, const string& label,
                      bool is_html_like) {
  string quoted_label = is_html_like ? util::StrCat("label=<", label, ">")
                                     : util::StrCat("label=\"", label, "\"");
  return util::StrCat("[", style, ", ", quoted_label, "]");
}

// Returns an AST as an HTML-like DOT label. If the AST is a container, every
// element in the container is a row of a table. Unlike standard HTML, the HTML
// implemented in DOT is whitespace sensitive so indents are spaces.
string ToDotIndent(const AST& ast, int indent) {
  if (ast.has_p_ast()) {
    string label = ast::ToStringRoot(ast, ast::PrintOption::kValue);
    AddEscapes(&label);
    return util::StrCat(string(indent, ' '), label);
  }
  std::vector<string> arg_str;
  for (const AST& arg : ast.c_ast().arg()) {
    arg_str.emplace_back(ToDotIndent(arg, indent + 2));
  }
  return util::StrCat(kTableHeader, "\n<tr><td>",
                      boost::algorithm::join(arg_str, "</td></tr>\n<tr><td>"),
                      "</td></tr>\n</table>");
}

}  // namespace

DotPrinter::DotPrinter()
    : node_attribute_(NodeAttribute), edge_attribute_(EdgeAttribute) {}

DotPrinter::DotPrinter(const AttributeFn& node_attribute,
                       const AttributeFn& edge_attribute)
    : node_attribute_(node_attribute), edge_attribute_(edge_attribute) {}

string DotPrinter::FileAttribute(const AST& ast) {
  string err;
  CHECK((ast::type::IsTyped(ast::type::MakeFile(), ast, &err)), "");
  if (!ast.has_c_ast() || ast.c_ast().arg_size() != 2) {
    return JoinAttributes(kFilenameStyle, "", false /*Do not use tags.*/);
  }
  const AST& path = ast.c_ast().arg(0);
  const int num_dirs = path.c_ast().arg_size();
  string filename = (num_dirs > 0) ? GetListElement(path, 0) : "";
  for (int i = 1; i < num_dirs; ++i) {
    util::StrAppend(&filename, R"(/\l)");
    util::StrAppend(&filename, string(2 * i, ' '), "\u21b3",
                    GetListElement(path, i));
  }
  const AST& filename_ast = ast.c_ast().arg(1);
  if (filename_ast.has_p_ast() && filename_ast.p_ast().has_val()) {
    util::StrAppend(&filename, R"(/\l)");
    util::StrAppend(&filename, string(2 * num_dirs, ' '), "\u21b3",
                    ast::value::GetString(filename_ast));
  }
  return JoinAttributes(kFilenameStyle, filename, false /*Do not use tags.*/);
}

string DotPrinter::IPAddressAttribute(const AST& ast) {
  string err;
  CHECK((ast::type::IsTyped(ast::type::MakeIPAddress(), ast, &err)), "");
  return JoinAttributes(kRemoteAddressStyle, ast::value::GetString(ast),
                        false /*Do not use tags.*/);
}

string DotPrinter::URLAttribute(const AST& ast) {
  string err;
  CHECK((ast::type::IsTyped(ast::type::MakeURL(), ast, &err)), "");
  return JoinAttributes(kRemoteAddressStyle, ast::value::GetString(ast),
                        false /*Do not use tags.*/);
}

string DotPrinter::NodeAttribute(const string& tag, const AST& ast) {
  if (tag == ast::kFileTag) {
    return FileAttribute(ast);
  } else if (tag == ast::kIPAddressTag) {
    return IPAddressAttribute(ast);
  } else if (tag == ast::kURLTag) {
    return URLAttribute(ast);
  } else {
    return JoinAttributes(kRoundedBoxStyle,
                          ToDotIndent(ast, 0),
                          true /*Use tags.*/);
  }
}

// Display an edge label only if there is a non-null AST on an edge.
string DotPrinter::EdgeAttribute(const string& tag, const AST& ast) {
  if (tag == ast::kPrecedesTag) {
    return JoinAttributes(kPrecedesStyle, "", false /*Do not use tags.*/);
  }
  if (ast::value::Isomorphic(ast, ast::value::MakeNull())) {
    return JoinAttributes(kDashedGrayEdge, "", false /*Do not use tags.*/);
  }
  return JoinAttributes(kDashedGrayEdge, ToDotIndent(ast, 0),
                        true /*Use tags.*/);
}

string DotPrinter::DotNode(NodeId node_id, const TaggedAST& tast) {
  string attr = tast.has_ast() ? node_attribute_(tast.tag(), tast.ast())
                               : JoinAttributes(kRoundedBoxStyle, tast.tag(),
                                                false /*Do not use tags.*/);
  return util::StrCat(std::to_string(node_id), " ", attr, ";");
}

string DotPrinter::DotEdge(NodeId source_id, NodeId target_id,
                           const TaggedAST& tast) {
  string attr = tast.has_ast() ? edge_attribute_(tast.tag(), tast.ast())
                               : JoinAttributes(kSolidGrayEdge, "",
                                                false /*Do not use tags.*/);
  return util::StrCat(std::to_string(source_id), " -> ",
                      std::to_string(target_id), " ", attr, ";");
}

string DotPrinter::AllNodesInDot(const LabeledGraph& graph) {
  string dot_nodes;
  string indent("  ");
  for (auto node_it = graph.NodeSetBegin(); node_it != graph.NodeSetEnd();
       ++node_it) {
    TaggedAST tast = graph.GetNodeLabel(*node_it);
    util::StrAppend(&dot_nodes, indent, DotNode(*node_it, tast), "\n");
  }
  return dot_nodes;
}

string DotPrinter::AllEdgesInDot(const LabeledGraph& graph) {
  string dot_edges;
  string indent("  ");
  for (auto edge_it = graph.EdgeSetBegin(); edge_it != graph.EdgeSetEnd();
       ++edge_it) {
    TaggedAST tast = graph.GetEdgeLabel(*edge_it);
    util::StrAppend(
        &dot_edges, indent,
        DotEdge(graph.Source(*edge_it), graph.Target(*edge_it), tast), "\n");
  }
  return dot_edges;
}

string DotPrinter::DotGraph(const LabeledGraph& graph) {
  string dot_graph = AllNodesInDot(graph);
  util::StrAppend(&dot_graph, AllEdgesInDot(graph));
  return util::StrCat("digraph logle_graph {\n", dot_graph, "}");
}

}  // namespace morphie
