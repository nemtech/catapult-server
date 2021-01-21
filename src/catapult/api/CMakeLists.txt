cmake_minimum_required(VERSION 3.13)

catapult_library_target(catapult.api)
target_link_libraries(catapult.api catapult.io catapult.ionet catapult.model)
