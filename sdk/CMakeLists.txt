cmake_minimum_required(VERSION 3.13)

include_directories(. ${PROJECT_SOURCE_DIR} ${PROJECT_SOURCE_DIR}/src)

add_subdirectory(src)
catapult_add_tests_subdirectory(tests)
