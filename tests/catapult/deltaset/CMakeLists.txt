cmake_minimum_required(VERSION 3.13)

if(MSVC)
	set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /bigobj")
endif()

catapult_test_executable_target_header_only(tests.catapult.deltaset nodeps test)
