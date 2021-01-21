cmake_minimum_required(VERSION 3.13)

catapult_library_target(catapult.handlers)
target_link_libraries(catapult.handlers catapult.io catapult.ionet catapult.model)
