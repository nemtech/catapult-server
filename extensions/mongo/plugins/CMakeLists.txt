cmake_minimum_required(VERSION 3.14)

set(PLUGIN_CATAPULT_LIBS catapult.mongo)

function(catapult_mongo_plugin_src_no_deps PLUGIN_BASE_NAME)
	catapult_shared_library_target(${PLUGIN_BASE_NAME} "${ARGN}")
	catapult_add_mongo_dependencies(${PLUGIN_BASE_NAME})
	target_link_libraries(${PLUGIN_BASE_NAME} ${PLUGIN_CATAPULT_LIBS})

	add_dependencies(mongo ${PLUGIN_BASE_NAME})
endfunction()

function(catapult_mongo_plugin_src_with_deps PLUGIN_BASE_NAME)
	# create a deps lib
	catapult_library_target(${PLUGIN_BASE_NAME}.deps "${ARGN}")
	catapult_add_mongo_dependencies(${PLUGIN_BASE_NAME}.deps)
	target_link_libraries(${PLUGIN_BASE_NAME}.deps ${PLUGIN_CATAPULT_LIBS})

	# create a plugin dll
	catapult_shared_library_target(${PLUGIN_BASE_NAME})
	target_link_libraries(${PLUGIN_BASE_NAME} ${PLUGIN_BASE_NAME}.deps)

	add_dependencies(mongo ${PLUGIN_BASE_NAME})
endfunction()

function(catapult_mongo_plugin_tests_no_deps TARGET_NAME)
	catapult_test_executable_target(${TARGET_NAME} mongo)
	catapult_add_mongo_dependencies(${TARGET_NAME})

	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}" PARENT_SCOPE)
endfunction()

function(catapult_mongo_plugin_tests_with_header_only_deps TARGET_NAME)
	string(REGEX MATCH "\.[a-z_]+$" PLUGIN_NAME ${TARGET_NAME})
	string(SUBSTRING ${PLUGIN_NAME} 1 -1 PLUGIN_NAME)

	catapult_test_executable_target(${TARGET_NAME} mongo "${ARGN}")
	catapult_add_mongo_dependencies(${TARGET_NAME})
	target_link_libraries(${TARGET_NAME} catapult.plugins.${PLUGIN_NAME}.deps)
	# there's dependency on plugin, so dependency on .deps should not be needed?
	# target_link_libraries(${TARGET_NAME} catapult.mongo.plugins.${PLUGIN_NAME}.deps)

	if(TARGET "tests.catapult.test.plugins.${PLUGIN_NAME}")
		target_link_libraries(${TARGET_NAME} tests.catapult.test.plugins.${PLUGIN_NAME})
	endif()

	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}" PARENT_SCOPE)
endfunction()

function(catapult_mongo_plugin_tests_with_deps TARGET_NAME)
	string(REGEX MATCH "\.[a-z]+$" PLUGIN_NAME ${TARGET_NAME})
	string(SUBSTRING ${PLUGIN_NAME} 1 -1 PLUGIN_NAME)

	catapult_mongo_plugin_tests_with_header_only_deps(${TARGET_NAME} "${ARGN}")
	target_link_libraries(${TARGET_NAME} tests.catapult.test.mongo.plugins.${PLUGIN_NAME})

	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}" PARENT_SCOPE)
endfunction()

function(catapult_mongo_plugin_test_library TARGET_NAME)
	catapult_add_gtest_dependencies()
	catapult_library_target(${TARGET_NAME})
	catapult_add_mongo_dependencies(${TARGET_NAME})
endfunction()

add_subdirectory(account_link)
add_subdirectory(aggregate)
add_subdirectory(lock_hash)
add_subdirectory(lock_secret)
add_subdirectory(lock_shared)
add_subdirectory(metadata)
add_subdirectory(mosaic)
add_subdirectory(multisig)
add_subdirectory(namespace)
add_subdirectory(restriction_account)
add_subdirectory(restriction_mosaic)
add_subdirectory(transfer)
