cmake_minimum_required(VERSION 3.14)

catapult_mongo_plugin_test_library(tests.catapult.test.mongo.plugins.locksecret)
target_link_libraries(tests.catapult.test.mongo.plugins.locksecret catapult.plugins.locksecret.deps catapult.mongo.plugins.locksecret)
