cmake_minimum_required(VERSION 3.13)

set(TARGET_NAME tests.catapult.int.stress)

catapult_int_test_executable_target(${TARGET_NAME} test)
target_link_libraries(${TARGET_NAME}
	catapult.cache_db
	catapult.harvesting
	catapult.plugins.hashcache
	catapult.plugins.hashcache.cache
	tests.catapult.test.local
	tests.catapult.test.nemesis)

set_property(TEST ${TARGET_NAME} PROPERTY LABELS Stress)

# add dependency on hash cache plugin
include_directories(../../../plugins/services/hashcache)
