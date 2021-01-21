cmake_minimum_required(VERSION 3.13)

catapult_library_target(catapult.tree)
target_link_libraries(catapult.tree catapult.crypto)
