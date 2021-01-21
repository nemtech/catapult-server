cmake_minimum_required(VERSION 3.13)

set(TARGET_NAME catapult.zeromq)

catapult_library_target(${TARGET_NAME})
target_link_libraries(${TARGET_NAME} catapult.model catapult.thread)
catapult_add_zeromq_dependencies(${TARGET_NAME})
