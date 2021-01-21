cmake_minimum_required(VERSION 3.13)

message("--- locating mongo dependencies ---")
find_package(MONGOCXX 3.6.1 EXACT REQUIRED)
find_package(MONGOC-1.0 1.17.2 EXACT REQUIRED)

message("mongocxx  ver: ${MONGOCXX_VERSION}")
message("mongoc    ver: ${MONGOC-1.0_VERSION}")
get_library_path("MONGOCXX_LIBRARY_PATH" mongo::mongocxx_shared)
message("mongocxx  lib: ${MONGOCXX_LIBRARY_PATH}")

### find and set mongo includes and libraries
function(catapult_add_mongo_dependencies TARGET_NAME)
	target_link_libraries(${TARGET_NAME} mongo::mongocxx_shared)
endfunction()

# used to define a catapult mongo int test executable by combining catapult_test_executable and catapult_target
function(catapult_int_test_executable_target TARGET_NAME)
	catapult_test_executable(${TARGET_NAME} ${ARGN})
	catapult_target(${TARGET_NAME})

	set_property(TEST ${TARGET_NAME} PROPERTY LABELS Database)

	# customize and export compiler options for gtest
	catapult_set_test_compiler_options()
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}" PARENT_SCOPE)
endfunction()

add_subdirectory(plugins)

catapult_define_extension_no_group(mongo)
catapult_add_mongo_dependencies(extension.mongo)

# add extension to mongo group
add_dependencies(mongo extension.mongo)
