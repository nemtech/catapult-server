cmake_minimum_required(VERSION 3.13)

catapult_test_executable_target(tests.catapult.local.recovery local test)
target_link_libraries(tests.catapult.local.recovery tests.catapult.test.nemesis)
