cmake_minimum_required(VERSION 3.13)

set(TARGET_NAME tests.catapult.int.node)

catapult_int_test_executable_target(${TARGET_NAME})
target_link_libraries(${TARGET_NAME} tests.catapult.int.node.test tests.catapult.test.nemesis catapult.local.server)
