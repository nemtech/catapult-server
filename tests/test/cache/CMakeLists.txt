cmake_minimum_required(VERSION 3.13)

catapult_library_target(tests.catapult.test.cache)
target_link_libraries(tests.catapult.test.cache tests.catapult.test.core catapult.cache catapult.cache_core catapult.cache_tx)
