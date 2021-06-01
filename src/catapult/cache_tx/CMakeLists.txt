cmake_minimum_required(VERSION 3.14)

catapult_library_target(catapult.cache_tx)
target_link_libraries(catapult.cache_tx catapult.utils)
