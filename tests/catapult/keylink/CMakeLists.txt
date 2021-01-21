cmake_minimum_required(VERSION 3.13)

catapult_test_executable_target_header_only(tests.catapult.keylink cache)
target_link_libraries(tests.catapult.keylink catapult.observers catapult.validators)
