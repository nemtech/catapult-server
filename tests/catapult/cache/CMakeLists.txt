cmake_minimum_required(VERSION 3.13)

catapult_test_executable_target(tests.catapult.cache cache test)
target_link_libraries(tests.catapult.cache catapult.crypto)
catapult_add_rocksdb_dependencies(tests.catapult.cache)
