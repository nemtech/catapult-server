cmake_minimum_required(VERSION 3.13)

catapult_library_target(catapult.io)
target_link_libraries(catapult.io catapult.config catapult.model)
