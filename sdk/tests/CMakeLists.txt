cmake_minimum_required(VERSION 3.13)

set(TARGET_NAME tests.catapult.sdk)

catapult_test_executable_target(${TARGET_NAME} core builders extensions parsers)
