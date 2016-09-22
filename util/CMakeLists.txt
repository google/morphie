# Description:
#   Generic algorithmic and data structure utilities.

add_library(util_csv csv.h csv.cc)
target_compile_options(util_csv PRIVATE -fexceptions)

add_library(util_logging STATIC logging.h logging.cc)

add_library(util_map_utils STATIC map_utils.h)
set_target_properties(util_map_utils PROPERTIES LINKER_LANGUAGE CXX)

add_library(util_status STATIC status.h status.cc)

add_library(util_string_utils STATIC string_utils.h string_utils.cc)

add_library(util_time_utils STATIC time_utils.h time_utils.cc)

