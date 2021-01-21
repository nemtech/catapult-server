cmake_minimum_required(VERSION 3.13)

set(TARGET_NAME tests.catapult.mongo.plugins.restrictionaccount)

include_directories(${PROJECT_SOURCE_DIR}/plugins/txes/restriction_account)
catapult_mongo_plugin_tests_with_deps(${TARGET_NAME} mappers)

add_subdirectory(int)
add_subdirectory(test)
