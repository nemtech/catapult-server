cmake_minimum_required(VERSION 3.13)

# used to define a catapult int test executable by combining catapult_test_executable and catapult_target
function(catapult_int_test_executable_target TARGET_NAME)
	catapult_test_executable(${TARGET_NAME} ${ARGN})
	catapult_target(${TARGET_NAME})

	set_property(TEST ${TARGET_NAME} PROPERTY LABELS Integration)

	# customize and export compiler options for gtest
	catapult_set_test_compiler_options()
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}" PARENT_SCOPE)
endfunction()

add_subdirectory(node)
add_subdirectory(stress)
