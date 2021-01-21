cmake_minimum_required(VERSION 3.13)

set(TARGET_NAME catapult.broker)

catapult_executable(${TARGET_NAME})
target_link_libraries(${TARGET_NAME} catapult.local.broker catapult.process)
catapult_target(${TARGET_NAME})

add_dependencies(server ${TARGET_NAME})
