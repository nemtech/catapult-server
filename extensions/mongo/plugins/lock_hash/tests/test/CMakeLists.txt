cmake_minimum_required(VERSION 3.14)

catapult_mongo_plugin_test_library(tests.catapult.test.mongo.plugins.lockhash)
target_link_libraries(tests.catapult.test.mongo.plugins.lockhash catapult.plugins.lockhash.deps catapult.mongo.plugins.lockhash)
