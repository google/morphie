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

// Provides an interface for rendering Graphviz dot graphs with the Graphviz
// server.
// Usage 1: Generate an SVG file using the 'dot' layout engine.
//   string graph;
//   // code to populate graph.
//   string url;
//   RendererConfig config;
//   ::util::Status s = DotRenderer::RenderWithGraphViz(config, graph, &url);
//   if (s.ok()) {
//     LOG(INFO) << "Access graph at:" << url;
//   }
// Usage 2: Generate a PNG file using the circo layout engine (places nodes on
// the circumference of a circle), and try to render exactly once with a 90
// second timeout.
//   string graph;
//   // code to populate graph.
//   string url;
//   RendererConfig config;
//   config.set_layout_engine("circo");
//   config.set_output_format("png");
//   config.set_min_timeout_seconds(90);
//   config.set_num_tries(1);
//   ::util::Status s = DotRenderer::RenderWithGraphViz(config, graph, &url);
//   if (s.ok()) {
//     LOG(INFO) << "Access graph at:" << url;
//   }
#ifndef THIRD_PARTY_LOGLE_DOT_RENDERER_H_
#define THIRD_PARTY_LOGLE_DOT_RENDERER_H_

#include <string>

#include "third_party/logle/renderer_config.proto.h"
#include "util/task/status.h"
#include "visualization/graphviz_server/proto/graphviz-server.pb.h"
#include "visualization/graphviz_server/public/graphviz-stubby-channel.h"

namespace tervuren {

class DotRenderer {
 public:
  // Renders the graph in 'dot_graph' using an RPC call to the Graphviz server.
  // The returned status object has one of the error codes below and the details
  // of the error are provided by ::util::Status::error_message().
  //   * INTERNAL - if connecting to the Graphviz server failed.
  //   * INVALID_ARGUMENT if any one of the conditions below is violated:
  //     - config.layout_engine is one of circo, dot, fdp, twopi.
  //     - config.output_format is one of png, ps, svg.
  //     - config.min_timeout_seconds() is non-negative.
  //     - config.num_tries() is non-negative.
  //   * UNKOWN if one of the conditions below occurs:
  //     - the string in dot_graph has syntax errors.
  //     - the Graphviz server timed out while rendering.
  //  - The method crashes if graph_url is null.
  static ::util::Status RenderWithGraphViz(const RendererConfig& config,
                                           const string& dot_graph,
                                           string* graph_url);

 private:
  DotRenderer() {}
  static ::util::Status ConfigureRequest(
      const RendererConfig& config, graphviz_server::RenderRequest* request);
  static ::util::Status Render(graphviz_server::RenderRequest* request,
                               graphviz_server::RenderResponse* response);

  DISALLOW_COPY_AND_ASSIGN(DotRenderer);
};

}  // namespace tervuren

#endif  // THIRD_PARTY_LOGLE_DOT_RENDERER_H_
