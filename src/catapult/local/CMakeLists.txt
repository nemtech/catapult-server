cmake_minimum_required(VERSION 3.13)

catapult_library_target(catapult.local)
target_link_libraries(catapult.local catapult.extensions)

add_subdirectory(broker)
add_subdirectory(recovery)
add_subdirectory(server)
