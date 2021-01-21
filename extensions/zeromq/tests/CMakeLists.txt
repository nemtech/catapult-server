cmake_minimum_required(VERSION 3.13)

set(TARGET_NAME tests.catapult.zeromq)

catapult_test_executable_target(${TARGET_NAME} core test)
catapult_add_zeromq_dependencies(${TARGET_NAME})

set_property(TEST ${TARGET_NAME} PROPERTY LABELS Integration)
