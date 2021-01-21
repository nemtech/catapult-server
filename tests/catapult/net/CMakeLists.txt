cmake_minimum_required(VERSION 3.13)

catapult_test_executable_target(tests.catapult.net net)
target_link_libraries(tests.catapult.net catapult.api)
