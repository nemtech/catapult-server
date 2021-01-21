cmake_minimum_required(VERSION 3.13)

catapult_library_target(catapult.local.recovery)
target_link_libraries(catapult.local.recovery catapult.local)
