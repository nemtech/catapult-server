cmake_minimum_required(VERSION 3.13)

catapult_test_executable_target(tests.catapult.local.server local)
target_link_libraries(tests.catapult.local.server tests.catapult.test.nemesis)
