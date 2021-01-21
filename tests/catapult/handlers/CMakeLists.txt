cmake_minimum_required(VERSION 3.13)

catapult_test_executable_target(tests.catapult.handlers net)
target_link_libraries(tests.catapult.handlers catapult.tree)
