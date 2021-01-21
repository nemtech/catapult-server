cmake_minimum_required(VERSION 3.13)

catapult_library_target(tests.catapult.test.nemesis)
target_link_libraries(tests.catapult.test.nemesis
	tests.catapult.test.local
	catapult.plugins.mosaic
	catapult.plugins.namespace
	catapult.plugins.transfer) # shared lib dependencies allow tests to create real transaction plugins

include_directories(../../../plugins/txes/mosaic)
include_directories(../../../plugins/txes/namespace)
