# Description:
#   Generic algorithmic and data structure utilities.

package(
    default_copts = [
        "-DPLATFORM_GOOGLE",
    ],
    default_visibility = ["//third_party/logle:internal"],
)

licenses(["notice"])  # Apache 2.0

exports_files(["LICENSE"])

# Path to Boost libraries.
BOOST = "//third_party/boost/do_not_include_from_google3_only_third_party/boost:boost"

# A parser for Comma Separated Values (CSV).
cc_library(
    name = "csv",
    srcs = ["csv.cc"],
    hdrs = ["csv.h"],
    deps = [
        BOOST,
        ":status",
        "//third_party/logle/base:string",
    ],
)

# Assertion checking utilities.
cc_library(
    name = "logging",
    srcs = ["logging.cc"],
    hdrs = ["logging.h"],
    deps = [
        "//third_party/logle/base:string",
    ],
)

# Error codes and status.
cc_library(
    name = "status",
    srcs = ["status.cc"],
    hdrs = ["status.h"],
    deps = [
        "//third_party/logle/base:string",
    ],
)

# String manipulation utilities.
cc_library(
    name = "string_utils",
    srcs = ["string_utils.cc"],
    hdrs = ["string_utils.h"],
    deps = [
        "//third_party/logle/base:string",
    ],
)

# Time utilities.
cc_library(
    name = "time_utils",
    srcs = ["time_utils.cc"],
    hdrs = ["time_utils.h"],
    deps = [
        "//third_party/logle/base:string",
    ],
)

# Tests of string utilities.

cc_test(
    name = "status_test",
    size = "small",
    srcs = ["status_test.cc"],
    deps = [
        ":status",
        "//third_party/logle:gtest",
        "//third_party/logle/base:string",
    ],
)

cc_test(
    name = "csv_test",
    size = "small",
    srcs = ["csv_test.cc"],
    deps = [
        BOOST,
        ":csv",
        "//third_party/logle:gtest",
    ],
)

cc_test(
    name = "string_utils_test",
    size = "small",
    srcs = ["string_utils_test.cc"],
    deps = [
        ":string_utils",
        "//third_party/logle:gtest",
        "//third_party/logle/base:string",
    ],
)

cc_test(
    name = "time_utils_test",
    size = "small",
    srcs = ["time_utils_test.cc"],
    deps = [
        ":time_utils",
        "//third_party/logle:gtest",
        "//third_party/logle/base:string",
    ],
)
