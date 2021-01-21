cmake_minimum_required(VERSION 3.13)

set(TARGET_NAME tests.catapult.int.mongo.plugins.lockhash)

catapult_int_test_executable_target(${TARGET_NAME} storages)
catapult_add_mongo_dependencies(${TARGET_NAME})
target_link_libraries(${TARGET_NAME}
	tests.catapult.test.mongo.plugins.lockhash
	tests.catapult.test.plugins.lockhash
	tests.catapult.test.cache
	tests.catapult.test.mongo
	catapult.mongo.plugins.lockhash
	catapult.plugins.lockhash.deps)
