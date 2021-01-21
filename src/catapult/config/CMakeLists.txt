cmake_minimum_required(VERSION 3.13)

catapult_library_target(catapult.config)
target_link_libraries(catapult.config catapult.ionet)
