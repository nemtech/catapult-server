cmake_minimum_required(VERSION 3.13)

set(TARGET_NAME tests.catapult.cache_db)

catapult_test_executable_target(${TARGET_NAME} nodeps test)
target_link_libraries(${TARGET_NAME} catapult.io)
catapult_add_rocksdb_dependencies(${TARGET_NAME})

set_property(TEST ${TARGET_NAME} PROPERTY LABELS Database_Rocks)
