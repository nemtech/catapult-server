cmake_minimum_required(VERSION 3.13)

set(TARGET_NAME catapult.cache_db)

catapult_library_target(${TARGET_NAME})
catapult_add_rocksdb_dependencies(${TARGET_NAME})
target_link_libraries(${TARGET_NAME} catapult.config catapult.tree)
