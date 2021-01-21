cmake_minimum_required(VERSION 3.13)

include_directories(${PROJECT_SOURCE_DIR} ${PROJECT_SOURCE_DIR}/src)

add_subdirectory(bench)
add_subdirectory(catapult)
add_subdirectory(test)
add_subdirectory(int)
