#include "third_party/logle/dot_renderer.h"

#include <memory>
#include "net/rpc/rpc-errorspace.h"
#include "net/rpc2/rpc2.h"
#include "util/regexp/re2/re2.h"
#include "util/task/codes.pb.h"

namespace third_party_logle {

::util::Status DotRenderer::RenderWithGraphViz(const RendererConfig& config,
                                               const string& dot_graph,
                                               string* graph_url) {
  CHECK_NOTNULL(graph_url);
  graphviz_server::RenderRequest request;
  graphviz_server::RenderResponse response;

  ::util::Status status = ConfigureRequest(config, &request);
  if (!status.ok()) {
    return status;
  }
  request.mutable_graph()->set_dot(dot_graph);

  int timeout = config.min_timeout_seconds();
  request.set_timeout_seconds(config.min_timeout_seconds());
  for (int i = 0; i < config.num_tries(); ++i) {
    status = Render(&request, &response);

    if (rpc::status::IsOk(status)) {
      *graph_url = response.url();
      return ::util::Status::OK;
    }
    if (!RE2::PartialMatch(status.error_message(), "timed out") &&
        !RE2::PartialMatch(status.error_message(), "could not connect"))  {
      return status;
    }
    timeout = 2 * timeout;
    request.set_timeout_seconds(timeout);
  }
  return status;
}

::util::Status DotRenderer::ConfigureRequest(
    const RendererConfig& config, graphviz_server::RenderRequest* request) {
  request->set_return_bytes(false);

  string layout = config.layout_engine();
  if (layout != "circo" && layout != "dot" && layout != "fdp" &&
      layout != "neato" && layout != "sfdp" && layout != "twopi") {
    return ::util::Status(::util::error::INVALID_ARGUMENT,
                          "Layout engine must be one of:"
                          " circo, dot, fdp, neato, sfdp, twopi.");
  }
  request->mutable_graph()->set_layout_engine(layout);

  string format = config.output_format();
  if (format == "dot") {
    request->set_format(graphviz_server::FORMAT_DOT);
  } else if (format == "png") {
    request->set_format(graphviz_server::FORMAT_PNG);
  } else if (format == "ps") {
    request->set_format(graphviz_server::FORMAT_PS);
  } else if (format == "svg") {
    request->set_format(graphviz_server::FORMAT_SVG);
  } else {
    return ::util::Status(::util::error::INVALID_ARGUMENT,
                          "Output format must be one of: png, ps, svg.");
  }

  if (config.min_timeout_seconds() < 0) {
    return ::util::Status(::util::error::INVALID_ARGUMENT,
                          "Min timeout must be non-negative.");
  }
  if (config.num_tries() < 0) {
    return ::util::Status(::util::error::INVALID_ARGUMENT,
                          "Number of tries must be non-negative.");
  }
  return ::util::Status::OK;
}

::util::Status DotRenderer::Render(graphviz_server::RenderRequest* request,
                                   graphviz_server::RenderResponse* response) {
  std::unique_ptr<graphviz_server::RenderServer> render_server(
      graphviz_server::NewRenderServerStub());
  if (render_server == nullptr) {
    return ::util::Status(::util::error::INTERNAL,
                          "Could not connect to Graphviz server");
  }

  RPC rpc;
  rpc.set_deadline(RPC::NO_DEADLINE);
  LOG(INFO) << "Connecting to Graphviz server with a timeout of "
            << request->timeout_seconds() << " seconds.";

  render_server->Render(&rpc, request, response, NULL);
  rpc.Wait();
  render_server.reset();

  return rpc.util_status();
}

}  // namespace third_party_logle
