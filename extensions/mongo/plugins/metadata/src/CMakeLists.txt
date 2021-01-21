cmake_minimum_required(VERSION 3.13)

set(PLUGIN_BASE_NAME catapult.mongo.plugins.metadata)

include_directories(${PROJECT_SOURCE_DIR}/plugins/txes/metadata)
catapult_mongo_plugin_src_with_deps(${PLUGIN_BASE_NAME} mappers storages)

# metadata.deps is needed because the mongo metadata cache storage needs the metadata cache
target_link_libraries(${PLUGIN_BASE_NAME}.deps catapult.plugins.metadata.deps)
