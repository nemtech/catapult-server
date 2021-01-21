cmake_minimum_required(VERSION 3.13)

catapult_library_target(catapult.cache_core)
target_link_libraries(catapult.cache_core catapult.cache catapult.state)
