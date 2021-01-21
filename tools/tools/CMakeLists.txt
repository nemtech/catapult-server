cmake_minimum_required(VERSION 3.13)

catapult_library_target(catapult.tools)
target_link_libraries(catapult.tools ${CORE_CATAPULT_LIBS} catapult.api catapult.config catapult.net catapult.sdk catapult.version)
