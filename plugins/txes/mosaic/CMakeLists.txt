cmake_minimum_required(VERSION 3.13)

set(PLUGIN_SDK_FOLDERS model state)
set(PLUGIN_DEPS_FOLDERS cache config observers validators)

include_directories(.)
add_subdirectory(src)
catapult_add_tests_subdirectory(tests)
