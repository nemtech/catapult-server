cmake_minimum_required(VERSION 3.13)

catapult_library_target(catapult.local.broker)
target_link_libraries(catapult.local.broker catapult.local)
