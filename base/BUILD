# Description:
#   Common headers and declarations.

package(
    default_copts = [
        "-DPLATFORM_GOOGLE",
    ],
    default_visibility = ["//third_party/logle:internal"],
)

licenses(["notice"])  # Apache 2.0

exports_files(["LICENSE"])

# Enforce std::string.
cc_library(
    name = "string",
    srcs = ["string.h"],
    hdrs = ["string.h"],
)
