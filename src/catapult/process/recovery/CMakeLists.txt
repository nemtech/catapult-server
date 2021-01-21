cmake_minimum_required(VERSION 3.13)

set(TARGET_NAME catapult.recovery)

catapult_executable(${TARGET_NAME})
target_link_libraries(${TARGET_NAME} catapult.local.recovery catapult.process)
catapult_target(${TARGET_NAME})

add_dependencies(server ${TARGET_NAME})
