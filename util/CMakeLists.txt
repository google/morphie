# Description:
#   Generic algorithmic and data structure utilities.

add_library(util_csv csv.h csv.cc)
target_compile_options(util_csv PRIVATE -fexceptions)

add_library(util_logging logging.h logging.cc)

add_library(util_status status.h status.cc)

add_library(util_string_utils string_utils.h string_utils.cc)

add_library(util_time_utils time_utils.h time_utils.cc)
