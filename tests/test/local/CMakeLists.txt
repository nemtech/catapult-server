cmake_minimum_required(VERSION 3.13)

catapult_library_target(tests.catapult.test.local)
target_link_libraries(tests.catapult.test.local
	tests.catapult.test.cache
	tests.catapult.test.net
	catapult.local.server)
