# Download and install Google Test libraries.
#
# Create a directory called 'googletest' in the build area and copy the file
# CMakeLists.txt.googletest there.
configure_file(CMakeLists.txt.googletest
	googletest/CMakeLists.txt)
# Invoke a separate run of cmake as an external command on the copied
# CMakeLists.txt file. This invocation ensures that googletest is unpacked and
# installed during configuration time. If the command below is not invoked, a
# separate script would have to be run to go through the build steps for
# dependencies in the right order.
set(gtest_download_dir "${CMAKE_BINARY_DIR}/third_party/googletest")
execute_process(COMMAND ${CMAKE_COMMAND} -G "${CMAKE_GENERATOR}" .
	WORKING_DIRECTORY ${gtest_download_dir} )
execute_process(COMMAND ${CMAKE_COMMAND} --build .
	WORKING_DIRECTORY ${gtest_download_dir} )
# Adds the build options 'gtest' and 'gtest_main', which are defined in the
# CMakeLists file of googletest to this build.
add_compile_options(-fexceptions)
add_subdirectory(${CMAKE_BINARY_DIR}/googletest-src
 	${CMAKE_BINARY_DIR}/googletest-build)

# Download and install the open source JSON parser.
#
# Create a directory called 'jsoncpp' in the build area and copy the file
# CMakeLists.txt.jsoncpp there.
configure_file(CMakeLists.txt.jsoncpp
	jsoncpp/CMakeLists.txt)
message(STATUS "DOWNLOADING JSONCPP")
set(jsoncpp_build_dir "${CMAKE_BINARY_DIR}/third_party/jsoncpp")
message(STATUS "RAN JSONCPP CMAKE in ${jsoncpp_build_dir}")
execute_process(COMMAND ${CMAKE_COMMAND} -G "${CMAKE_GENERATOR}" .
	WORKING_DIRECTORY ${jsoncpp_build_dir} )
message(STATUS "RAN JSONCPP CMAKE in ${jsoncpp_build_dir}")
set(CMAKE_INSTALL_COMPONENT "${jsoncpp_build_dir}")
execute_process(COMMAND ${CMAKE_COMMAND} --build .
	WORKING_DIRECTORY ${jsoncpp_build_dir} )
message(STATUS "RAN JSONCPP MAKE")
message(STATUS "DONE  WITH JSON")

# Download and install the gflags command line library.
#
# Create a directory called 'gflags' in the build area and copy the file
# CMakeLists.txt.gflags there.
configure_file(CMakeLists.txt.gflags
	gflags/CMakeLists.txt)
set(gflags_download_dir "${CMAKE_BINARY_DIR}/third_party/gflags")
execute_process(COMMAND ${CMAKE_COMMAND} -G "${CMAKE_GENERATOR}" .
	WORKING_DIRECTORY ${gflags_download_dir} )
execute_process(COMMAND ${CMAKE_COMMAND} --build .
	WORKING_DIRECTORY ${gflags_download_dir} )
# Adds the build options 'gtest' and 'gtest_main', which are defined in the
# CMakeLists file of googletest to this build.
# add_subdirectory(${CMAKE_BINARY_DIR}/gflags-src
#	${CMAKE_BINARY_DIR}/gflags-build)
