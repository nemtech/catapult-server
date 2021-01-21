cmake_minimum_required(VERSION 3.13)

catapult_library_target(catapult.subscribers)
target_link_libraries(catapult.subscribers catapult.cache catapult.cache_tx catapult.config catapult.validators)
