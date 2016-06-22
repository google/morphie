# Description:
#   logle: a tool for constructing labeled graphs from log data.
package(
    default_copts = [
        "-DPLATFORM_GOOGLE",
    ],
    default_hdrs_check = "loose",
    default_visibility = [":internal"],
    features = [
        "-parse_headers",
        "no_layering_check",
    ],
)

licenses(["notice"])  # Apache 2.0

exports_files(["LICENSE"])

# The build extension called portable_proto_library ensures that protos are
# compiled with the open source protocol buffer compiler.
load("//net/proto2/contrib/portable/cc:portable_proto_build_defs.bzl", "portable_proto_library")

# Path to Boost libraries.
BOOST = "//third_party/boost/do_not_include_from_google3_only_third_party/boost:boost"

# Required because the Boost Regex libraries have to be linked in separately.
BOOST_REGEX = "//third_party/boost/do_not_include_from_google3_only_third_party/boost/libs/regex:regex"

# Path to the open source protobuf compiler implementation.
PROTO_LIB = "//third_party/protobuf:protobuf-lite"

package_group(
    name = "internal",
    packages = ["//third_party/logle/..."],
)

cc_library(
    name = "gtest",
    testonly = 1,
    hdrs = ["gtest.h"],
    copts = [
        "-DPLATFORM_GOOGLE",
    ],
    deps = [
        "//testing/base/public:gunit",
        "//testing/base/public:gunit_main",
    ],
)

# Packages for the Curio analyzer.

cc_library(
    name = "curio_defs",
    srcs = ["curio_defs.cc"],
    hdrs = ["curio_defs.h"],
    deps = [],
)

cc_library(
    name = "stream_dependency_graph",
    srcs = ["stream_dependency_graph.cc"],
    hdrs = [
        "graph_interface.h",
        "stream_dependency_graph.h",
    ],
    deps = [
        ":curio_defs",
        ":dot_printer",
        ":labeled_graph",
        ":type",
        ":type_checker",
        ":value",
        "//third_party/logle/util:logging",
        "//third_party/logle/util:status",
        "//third_party/logle/util:string_utils",
    ],
)

cc_library(
    name = "curio_analyzer",
    srcs = ["curio_analyzer.cc"],
    hdrs = ["curio_analyzer.h"],
    deps = [
        ":stream_dependency_graph",
        "//third_party/jsoncpp:json",
        "//third_party/logle/util:logging",
        "//third_party/logle/util:status",
        "//third_party/logle/util:string_utils",
    ],
)

# Packages for the Plaso analyzer.

portable_proto_library(
    name = "ast_proto",
    srcs = ["ast.proto"],
    config_string = "allow_all : true",
    header_outs = ["ast.pb.h"],
)

cc_library(
    name = "ast",
    srcs = ["ast.cc"],
    hdrs = ["ast.h"],
    deps = [
        ":ast_proto",
        BOOST,
        "//third_party/logle/base:string",
        "//third_party/logle/util:string_utils",
        "//third_party/logle/util:time_utils",
    ],
)

cc_library(
    name = "type_checker",
    srcs = ["type_checker.cc"],
    hdrs = ["type_checker.h"],
    deps = [
        ":ast",
        ":ast_proto",
        ":value_checker",
        "//third_party/logle/base:string",
        "//third_party/logle/util:logging",
        "//third_party/logle/util:string_utils",
    ],
)

cc_library(
    name = "type",
    srcs = ["type.cc"],
    hdrs = ["type.h"],
    deps = [
        ":ast",
        ":ast_proto",
        ":type_checker",
    ],
)

cc_library(
    name = "value_checker",
    srcs = ["value_checker.cc"],
    hdrs = ["value_checker.h"],
    deps = [
        ":ast",
        ":ast_proto",
        "//third_party/logle/base:string",
        "//third_party/logle/util:logging",
        "//third_party/logle/util:string_utils",
    ],
)

cc_library(
    name = "value",
    srcs = ["value.cc"],
    hdrs = ["value.h"],
    deps = [
        ":ast",
        ":ast_proto",
        ":type_checker",
        ":value_checker",
        "//third_party/logle/util:logging",
        "//third_party/logle/util:time_utils",
    ],
)

cc_library(
    name = "labeled_graph",
    srcs = ["labeled_graph.cc"],
    hdrs = ["labeled_graph.h"],
    deps = [
        ":ast_proto",
        ":type_checker",
        BOOST,
        "//third_party/boost/allowed",
        "//third_party/logle/base:string",
        "//third_party/logle/util:logging",
        "//third_party/logle/util:status",
        "//third_party/logle/util:string_utils",
    ],
)

cc_library(
    name = "graph_transformer",
    srcs = ["graph_transformer.cc"],
    hdrs = ["graph_transformer.h"],
    deps = [
        ":labeled_graph",
        ":type",
        ":value",
        "//third_party/logle/util:logging",
        "//third_party/logle/util:status",
        "//third_party/logle/util:string_utils",
    ],
)

cc_library(
    name = "test_graphs",
    srcs = ["test_graphs.cc"],
    hdrs = ["test_graphs.h"],
    deps = [
        ":dot_printer",
        ":labeled_graph",
        ":type",
        ":value",
        "//third_party/logle/util:logging",
    ],
)

cc_library(
    name = "plaso_defs",
    srcs = ["plaso_defs.cc"],
    hdrs = ["plaso_defs.h"],
    deps = [],
)

portable_proto_library(
    name = "plaso_event_proto",
    srcs = ["plaso_event.proto"],
    config_string = "allow_all : true",
    header_outs = ["plaso_event.pb.h"],
)

cc_library(
    name = "plaso_event",
    srcs = ["plaso_event.cc"],
    hdrs = ["plaso_event.h"],
    deps = [
        ":ast_proto",
        ":plaso_defs",
        ":plaso_event_proto",
        ":type",
        ":value",
        "//third_party/jsoncpp:json",
        "//third_party/logle/base:string",
        "//third_party/logle/util:logging",
        "//third_party/logle/util:string_utils",
        "//third_party/logle/util:time_utils",
    ],
)

cc_library(
    name = "plaso_event_graph",
    srcs = ["plaso_event_graph.cc"],
    hdrs = [
        "graph_interface.h",
        "plaso_event_graph.h",
    ],
    deps = [
        ":ast",
        ":ast_proto",
        ":dot_printer",
        ":labeled_graph",
        ":plaso_event",
        ":plaso_event_proto",
        ":type",
        ":type_checker",
        BOOST,
        "//third_party/jsoncpp:json",
        "//third_party/logle/util:logging",
        "//third_party/logle/util:status",
        "//third_party/logle/util:string_utils",
        "//third_party/logle/util:time_utils",
    ],
)

cc_library(
    name = "dot_printer",
    srcs = ["dot_printer.cc"],
    hdrs = ["dot_printer.h"],
    deps = [
        ":ast",
        ":type",
        ":type_checker",
        ":value",
        BOOST,
        "//third_party/boost/allowed",
        "//third_party/logle/util:logging",
        "//third_party/logle/util:status",
        "//third_party/logle/util:string_utils",
    ],
)

cc_library(
    name = "tf_graph_exporter",
    srcs = ["tf_graph_exporter.cc"],
    hdrs = ["tf_graph_exporter.h"],
    deps = [
        ":ast",
        ":ast_proto",
        ":labeled_graph",
        BOOST,
        "//third_party/logle/base:string",
        "//third_party/logle/util:string_utils",
        "//third_party/tensorflow/core:protos_all",
    ],
)

cc_library(
    name = "plaso_analyzer",
    srcs = ["plaso_analyzer.cc"],
    hdrs = ["plaso_analyzer.h"],
    deps = [
        ":plaso_defs",
        ":plaso_event",
        ":plaso_event_graph",
        "//third_party/jsoncpp:json",
        "//third_party/logle/base:string",
        "//third_party/logle/util:status",
        "//third_party/logle/util:string_utils",
    ],
)

cc_library(
    name = "account_access_defs",
    srcs = ["account_access_defs.cc"],
    hdrs = ["account_access_defs.h"],
    deps = [],
)

cc_library(
    name = "account_access_graph",
    srcs = ["account_access_graph.cc"],
    hdrs = [
        "account_access_graph.h",
        "graph_interface.h",
    ],
    deps = [
        ":account_access_defs",
        ":dot_printer",
        ":labeled_graph",
        ":type",
        ":type_checker",
        ":value",
        "//third_party/logle/base:string",
        "//third_party/logle/util:logging",
        "//third_party/logle/util:status",
        "//third_party/logle/util:string_utils",
    ],
)

cc_library(
    name = "account_access_analyzer",
    srcs = ["account_access_analyzer.cc"],
    hdrs = ["account_access_analyzer.h"],
    deps = [
        ":account_access_defs",
        ":account_access_graph",
        "//third_party/logle/base:string",
        "//third_party/logle/util:csv",
        "//third_party/logle/util:logging",
        "//third_party/logle/util:status",
        "//third_party/logle/util:string_utils",
    ],
)

portable_proto_library(
    name = "analysis_options_proto",
    srcs = ["analysis_options.proto"],
    config_string = "allow_all : true",
    header_outs = ["analysis_options.pb.h"],
)

cc_library(
    name = "frontend",
    srcs = ["frontend.cc"],
    hdrs = ["frontend.h"],
    deps = [
        ":account_access_analyzer",
        ":analysis_options_proto",
        ":curio_analyzer",
        ":plaso_analyzer",
        "//third_party/jsoncpp:json",
        "//third_party/logle/base:string",
        "//third_party/logle/util:string_utils",
    ],
)

cc_binary(
    name = "logle",
    srcs = ["logle.cc"],
    deps = [
        ":frontend",
        PROTO_LIB,
        "//third_party/logle/base:string",
        "//third_party/logle/util:status",
    ],
)

# Tests for the Curio analyzer.

cc_test(
    name = "stream_dependency_graph_test",
    size = "small",
    srcs = ["stream_dependency_graph_test.cc"],
    deps = [
        ":gtest",
        ":stream_dependency_graph",
    ],
)

cc_test(
    name = "curio_analyzer_test",
    size = "small",
    srcs = ["curio_analyzer_test.cc"],
    deps = [
        ":curio_analyzer",
        ":gtest",
        "//third_party/jsoncpp:json",
        "//third_party/logle/util:status",
    ],
)

# Tests for the Plaso analyzer.

cc_test(
    name = "ast_test",
    size = "small",
    srcs = ["ast_test.cc"],
    deps = [
        ":ast",
        ":gtest",
        "//third_party/logle/base:string",
    ],
)

cc_test(
    name = "type_checker_test",
    size = "small",
    srcs = ["type_checker_test.cc"],
    deps = [
        ":gtest",
        ":type_checker",
        "//third_party/logle/base:string",
    ],
)

cc_test(
    name = "type_test",
    size = "small",
    srcs = ["type_test.cc"],
    deps = [
        ":gtest",
        ":type",
    ],
)

cc_test(
    name = "value_checker_test",
    size = "small",
    srcs = ["value_checker_test.cc"],
    deps = [
        ":ast",
        ":gtest",
        ":value_checker",
        "//third_party/logle/base:string",
    ],
)

cc_test(
    name = "value_test",
    size = "small",
    srcs = ["value_test.cc"],
    deps = [
        ":gtest",
        ":type",
        ":type_checker",
        ":value",
        ":value_checker",
    ],
)

cc_test(
    name = "labeled_graph_test",
    size = "small",
    srcs = ["labeled_graph_test.cc"],
    deps = [
        ":ast_proto",
        ":gtest",
        ":labeled_graph",
        ":type",
        ":type_checker",
        ":value",
        ":value_checker",
        "//third_party/logle/base:string",
        "//third_party/logle/util:status",
    ],
)

cc_test(
    name = "test_graphs_test",
    size = "small",
    srcs = ["test_graphs_test.cc"],
    deps = [
        ":gtest",
        ":test_graphs",
    ],
)

cc_test(
    name = "graph_transformer_test",
    size = "small",
    srcs = ["graph_transformer_test.cc"],
    deps = [
        ":ast",
        ":graph_transformer",
        ":gtest",
        ":labeled_graph",
        ":test_graphs",
        ":type",
        ":value",
        "//third_party/logle/util:string_utils",
    ],
)

cc_test(
    name = "dot_printer_test",
    size = "small",
    srcs = ["dot_printer_test.cc"],
    deps = [
        ":ast",
        ":ast_proto",
        ":dot_printer",
        ":gtest",
        ":labeled_graph",
        ":plaso_event",
        ":type",
        ":value",
        BOOST_REGEX,
        "//third_party/logle/base:string",
        "//third_party/logle/util:status",
    ],
)

cc_test(
    name = "tf_graph_exporter_test",
    size = "small",
    srcs = ["tf_graph_exporter_test.cc"],
    deps = [
        ":ast",
        ":ast_proto",
        ":gtest",
        ":labeled_graph",
        ":plaso_event",
        ":tf_graph_exporter",
        ":type",
        ":value",
        "//third_party/logle/base:string",
        "//third_party/logle/util:status",
    ],
)

cc_test(
    name = "plaso_event_test",
    size = "small",
    srcs = ["plaso_event_test.cc"],
    deps = [
        ":gtest",
        ":plaso_event",
        ":plaso_event_proto",
        ":type",
        ":type_checker",
        "//third_party/logle/base:string",
        "//third_party/logle/util:time_utils",
    ],
)

cc_test(
    name = "plaso_event_graph_test",
    size = "small",
    srcs = ["plaso_event_graph_test.cc"],
    deps = [
        ":gtest",
        ":plaso_event",
        ":plaso_event_graph",
        "//third_party/logle/util:status",
    ],
)

cc_test(
    name = "plaso_analyzer_test",
    size = "small",
    srcs = ["plaso_analyzer_test.cc"],
    deps = [
        ":gtest",
        ":plaso_analyzer",
        ":plaso_defs",
        "//third_party/logle/base:string",
        "//third_party/logle/util:csv",
        "//third_party/logle/util:status",
        "//third_party/logle/util:string_utils",
    ],
)

cc_test(
    name = "account_access_graph_test",
    size = "small",
    srcs = ["account_access_graph_test.cc"],
    deps = [
        ":account_access_defs",
        ":account_access_graph",
        ":gtest",
    ],
)

cc_test(
    name = "account_access_analyzer_test",
    size = "small",
    srcs = ["account_access_analyzer_test.cc"],
    deps = [
        ":account_access_analyzer",
        ":gtest",
        "//third_party/logle/util:csv",
    ],
)
