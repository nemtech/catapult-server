cmake_minimum_required(VERSION 3.13)

set(TARGET_NAME tests.catapult.mongo.plugins.mosaic)

include_directories(${PROJECT_SOURCE_DIR}/plugins/txes/mosaic)
catapult_mongo_plugin_tests_with_deps(${TARGET_NAME} mappers storages)

add_subdirectory(int)
add_subdirectory(test)
