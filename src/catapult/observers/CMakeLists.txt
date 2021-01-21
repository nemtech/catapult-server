cmake_minimum_required(VERSION 3.13)

catapult_library_target(catapult.observers)
target_link_libraries(catapult.observers catapult.cache)
