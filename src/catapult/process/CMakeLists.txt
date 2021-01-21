cmake_minimum_required(VERSION 3.13)

catapult_library_target(catapult.process)
target_link_libraries(catapult.process catapult.version)

add_subdirectory(broker)
add_subdirectory(recovery)
add_subdirectory(server)

install(TARGETS catapult.server catapult.broker catapult.recovery)
