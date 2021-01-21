cmake_minimum_required(VERSION 3.13)

catapult_test_executable_target(tests.catapult.local.broker local test)
target_link_libraries(tests.catapult.local.broker catapult.plugins.signature) # allow signature to be loaded implicitly
