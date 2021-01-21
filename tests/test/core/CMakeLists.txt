cmake_minimum_required(VERSION 3.13)

catapult_library_target(tests.catapult.test.core mocks)
target_link_libraries(tests.catapult.test.core
	tests.catapult.test.nodeps
	${CORE_CATAPULT_LIBS}
	catapult.sdk)
