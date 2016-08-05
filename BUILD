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

# Utilities

# A parser for Comma Separated Values (CSV).
cc_library(
    name = "util_csv",
    srcs = ["util/csv.cc"],
    hdrs = [
        "base/string.h",
        "util/csv.h",
    ],
    includes = ["."],
    deps = [
        BOOST,
        ":util_status",
    ],
)

# Assertion checking utilities.
cc_library(
    name = "util_logging",
    srcs = ["util/logging.cc"],
    hdrs = [
        "base/string.h",
        "util/logging.h",
    ],
    includes = ["."],
)

# Error codes and status.
cc_library(
    name = "util_status",
    srcs = ["util/status.cc"],
    hdrs = [
        "base/string.h",
        "util/status.h",
    ],
    includes = ["."],
)

# String manipulation utilities.
cc_library(
    name = "util_string_utils",
    srcs = ["util/string_utils.cc"],
    hdrs = [
        "base/string.h",
        "util/string_utils.h",
    ],
    includes = ["."],
)

# Time utilities.
cc_library(
    name = "util_time_utils",
    srcs = ["util/time_utils.cc"],
    hdrs = [
        "base/string.h",
        "util/time_utils.h",
    ],
    includes = ["."],
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
        ":util_logging",
        ":util_status",
        ":util_string_utils",
        ":value",
    ],
)

cc_library(
    name = "curio_analyzer",
    srcs = ["curio_analyzer.cc"],
    hdrs = ["curio_analyzer.h"],
    deps = [
        ":stream_dependency_graph",
        ":util_logging",
        ":util_status",
        ":util_string_utils",
        "//third_party/jsoncpp:json",
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
    hdrs = [
        "ast.h",
        "base/string.h",
    ],
    includes = ["."],
    deps = [
        ":ast_proto",
        BOOST,
        ":util_string_utils",
        ":util_time_utils",
    ],
)

cc_library(
    name = "type_checker",
    srcs = ["type_checker.cc"],
    hdrs = [
        "base/string.h",
        "type_checker.h",
    ],
    includes = ["."],
    deps = [
        ":ast",
        ":ast_proto",
        ":util_logging",
        ":util_string_utils",
        ":value_checker",
    ],
)

cc_library(
    name = "type",
    srcs = ["type.cc"],
    hdrs = [
        "base/string.h",
        "type.h",
    ],
    includes = ["."],
    deps = [
        ":ast",
        ":ast_proto",
        ":type_checker",
    ],
)

cc_library(
    name = "value_checker",
    srcs = ["value_checker.cc"],
    hdrs = [
        "base/string.h",
        "value_checker.h",
    ],
    includes = ["."],
    deps = [
        ":ast",
        ":ast_proto",
        ":util_logging",
        ":util_string_utils",
    ],
)

cc_library(
    name = "value",
    srcs = ["value.cc"],
    hdrs = [
        "base/string.h",
        "value.h",
    ],
    includes = ["."],
    deps = [
        ":ast",
        ":ast_proto",
        ":type_checker",
        ":util_logging",
        ":util_time_utils",
        ":value_checker",
    ],
)

cc_library(
    name = "labeled_graph",
    srcs = ["labeled_graph.cc"],
    hdrs = [
        "base/string.h",
        "labeled_graph.h",
    ],
    deps = [
        ":ast_proto",
        ":type_checker",
        BOOST,
        ":util_logging",
        ":util_status",
        ":util_string_utils",
        "//third_party/boost/allowed",
    ],
)

cc_library(
    name = "graph_transformer",
    srcs = ["graph_transformer.cc"],
    hdrs = ["graph_transformer.h"],
    deps = [
        ":labeled_graph",
        ":type",
        ":util_logging",
        ":util_status",
        ":util_string_utils",
        ":value",
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
        ":util_logging",
        ":value",
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
    hdrs = [
        "base/string.h",
        "plaso_event.h",
    ],
    deps = [
        ":ast_proto",
        ":plaso_defs",
        ":plaso_event_proto",
        ":type",
        ":util_logging",
        ":util_string_utils",
        ":util_time_utils",
        ":value",
        "//third_party/jsoncpp:json",
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
        ":util_logging",
        ":util_status",
        ":util_string_utils",
        ":util_time_utils",
        "//third_party/jsoncpp:json",
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
        ":util_logging",
        ":util_status",
        ":util_string_utils",
        "//third_party/boost/allowed",
    ],
)

cc_library(
    name = "tf_graph_exporter",
    srcs = ["tf_graph_exporter.cc"],
    hdrs = [
        "base/string.h",
        "tf_graph_exporter.h",
    ],
    deps = [
        ":ast",
        ":ast_proto",
        ":labeled_graph",
        BOOST,
        ":util_string_utils",
        "//third_party/tensorflow/core:protos_all",
    ],
)

cc_library(
    name = "plaso_analyzer",
    srcs = ["plaso_analyzer.cc"],
    hdrs = [
        "base/string.h",
        "plaso_analyzer.h",
    ],
    deps = [
        ":json_reader",
        ":plaso_defs",
        ":plaso_event",
        ":plaso_event_graph",
        ":util_status",
        ":util_string_utils",
        "//third_party/jsoncpp:json",
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
        "base/string.h",
        "graph_interface.h",
    ],
    deps = [
        ":account_access_defs",
        ":dot_printer",
        ":labeled_graph",
        ":type",
        ":type_checker",
        ":util_logging",
        ":util_status",
        ":util_string_utils",
        ":value",
    ],
)

cc_library(
    name = "account_access_analyzer",
    srcs = ["account_access_analyzer.cc"],
    hdrs = [
        "account_access_analyzer.h",
        "base/string.h",
    ],
    deps = [
        ":account_access_defs",
        ":account_access_graph",
        ":util_csv",
        ":util_logging",
        ":util_status",
        ":util_string_utils",
    ],
)

portable_proto_library(
    name = "analysis_options_proto",
    srcs = ["analysis_options.proto"],
    config_string = "allow_all : true",
    header_outs = ["analysis_options.pb.h"],
)

cc_library(
    name = "json_reader",
    srcs = ["json_reader.cc"],
    hdrs = [
        "base/string.h",
        "json_reader.h",
    ],
    includes = ["."],
    deps = [
        ":util_logging",
        "//third_party/jsoncpp:json",
    ],
)

cc_library(
    name = "frontend",
    srcs = ["frontend.cc"],
    hdrs = [
        "base/string.h",
        "frontend.h",
    ],
    deps = [
        ":account_access_analyzer",
        ":analysis_options_proto",
        ":curio_analyzer",
        ":json_reader",
        ":plaso_analyzer",
        ":util_logging",
        ":util_string_utils",
        "//third_party/jsoncpp:json",
    ],
)

cc_binary(
    name = "logle",
    srcs = ["logle.cc"],
    deps = [
        ":frontend",
        PROTO_LIB,
        ":util_status",
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
        ":util_status",
        "//third_party/jsoncpp:json",
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
    ],
)

cc_test(
    name = "type_checker_test",
    size = "small",
    srcs = ["type_checker_test.cc"],
    deps = [
        ":gtest",
        ":type_checker",
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
        ":util_status",
        ":value",
        ":value_checker",
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
        ":util_string_utils",
        ":value",
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
        ":util_status",
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
        ":util_status",
        ":value",
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
        ":util_time_utils",
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
        ":util_status",
    ],
)

cc_test(
    name = "plaso_analyzer_test",
    size = "small",
    srcs = [
        "base/string.h",
        "plaso_analyzer_test.cc",
    ],
    includes = ["."],
    deps = [
        ":gtest",
        ":json_reader",
        ":plaso_analyzer",
        ":plaso_defs",
        ":util_csv",
        ":util_status",
        ":util_string_utils",
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
        ":util_csv",
    ],
)
