cmake_minimum_required(VERSION 3.13)

set(PLUGIN_SDK_FOLDERS model)
set(PLUGIN_DEPS_FOLDERS config validators)

include_directories(.)
add_subdirectory(src)
catapult_add_tests_subdirectory(tests)
