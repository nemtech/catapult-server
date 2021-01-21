cmake_minimum_required(VERSION 3.13)

catapult_library_target(catapult.model)
target_link_libraries(catapult.model catapult.crypto)
