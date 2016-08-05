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

// This file provides utilities for rendering nodes, edges and entire graphs in
// GraphViz DOT format. The functions here implement default visualizations and
// provide an API for developing custom visualizations.
//
// Example 1. Generate a DOT representation using default settings.
//  LabeledGraph graph;
//  // Code that constructs a graph.
//  string dot_graph = DotPrinter().DotGraph(graph);
//
// Example 2. The client here is using a graph in which nodes are usernames or
// filenames. A client-defined visualization NodeRenderer() will be used for
// nodes but the default visualization will be used for edges.
//
//   string CustomRenderer(const string& tag, const AST& ast) { ... }
//
//   LabeledGraph graph;
//   ConstructGraph(&graph);
//   DotPrinter dot_printer(CustomRenderer, DotPrinter::EdgeAttribute);
//   string dot_graph = dot_printer.DotGraph(graph);
#ifndef THIRD_PARTY_LOGLE_DOT_PRINTER_H_
#define THIRD_PARTY_LOGLE_DOT_PRINTER_H_

#include <functional>

#include "third_party/logle/ast.pb.h"
#include "base/string.h"
#include "labeled_graph.h"

namespace tervuren {

// An attribute function returns a GraphViz DOT attribute for a node or an edge.
// The first argument represents a tag and the second is an abstract syntax tree
// (AST) representing a label. See ast.proto for more on ASTs.
using AttributeFn = std::function<string(const string&, const AST&)>;

// The DotPrinter class provides an API for generating GraphViz DOT
// representations of nodes, edges and graphs. The API allows for default
// visualization of edge and node labels.  These functions assume that specific
// information, such as a filename or a URL, is represented by ASTs with a
// globally unique tag and type. For example, ASTs for filenames are assumed to
// have a tag "Filename" and are assumed to be lists of strings. The utility
// functions generate DOT representations for predefined combinations of a tag
// and a type.
//
// The GraphViz DOT format declares nodes and edges and their attributes.
//
//  Example 3. A GraphViz DOT graph. The attributes are between square-brackets.
//  digraph ex3 {
//    a [shape=Box, label="Rectangle"];  // Node declaration
//    b [shape=Circle, label="Ellipse"];
//    a -> b [style=dashed]; // Edge declaration
//   }
//
// To enable customizable visualization, this API provides functions for
// generating an entire DOT graph, generating node and edge declarations with
// attributes, or generating only attributes. The constants define certain node
// and edge styles, which are part of an attribute definition.
class DotPrinter {
 public:
  // The constructor below, which takes no arguments, sets the node attribute
  // function to be DotPrinter::NodeAttribute and the edge attribute function to
  // be DotPrinter::EdgeAttribute.
  DotPrinter();

  // This constructor uses the arguments to generate node and edge attributes.
  // It is the responsibility of the client to ensure that these attribute
  // functions generate syntactically correct DOT attributes.
  DotPrinter(const AttributeFn& node_attribute,
             const AttributeFn& edge_attribute);

  // The XXXAttribute functions generate predefined node/edge attributes.
  //
  // Requires that 'ast' is of the type ast::type::MakeFilename().
  static string FileAttribute(const AST& ast);
  // Requires that 'ast' is of the type ast::type::MakeIPAddress().
  static string IPAddressAttribute(const AST& ast);
  // Requires that 'ast' is of the type ast::type::MakeURL().
  static string URLAttribute(const AST& ast);
  // Returns a predefined node attribute if one is defined for 'tag', and
  // otherwise returns a DOT node whose label is 'ast' as a string.
  static string NodeAttribute(const string& tag, const AST& ast);
  // Returns a predefined node attribute if one is defined for 'tag', and
  // otherwise returns a DOT node whose label is 'ast' as a string.
  static string EdgeAttribute(const string& tag, const AST& ast);

  // Returns a DOT node/edge declaration that is terminated with a semi-colon
  // but not a newline. The attribute is chosen using 'ast.tag()'.
  string DotNode(NodeId node_id, const TaggedAST& ast);
  string DotEdge(NodeId source_id, NodeId target_id, const TaggedAST& ast);

  // Returns a DOT declaration of all nodes/edges in the given graph. If the
  // graph has no nodes, AllNodesInDot returns the empty string. If the graph
  // has no edges, AllEdgesInDot returns the empty string.
  string AllNodesInDot(const LabeledGraph& graph);
  string AllEdgesInDot(const LabeledGraph& graph);

  // Returns a DOT representation of the graph. The returned string is not
  // newline terminated.
  string DotGraph(const LabeledGraph& graph);

 private:
  // The function used to generate node attributes.
  AttributeFn node_attribute_;
  // The function used to generate edge attributes.
  AttributeFn edge_attribute_;
};  // class DotPrinter

}  // namespace tervuren
#endif  // THIRD_PARTY_LOGLE_DOT_PRINTER_H_
