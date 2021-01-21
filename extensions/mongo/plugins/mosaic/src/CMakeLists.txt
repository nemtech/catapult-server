cmake_minimum_required(VERSION 3.13)

set(PLUGIN_BASE_NAME catapult.mongo.plugins.mosaic)

include_directories(${PROJECT_SOURCE_DIR}/plugins/txes/mosaic)
catapult_mongo_plugin_src_with_deps(${PLUGIN_BASE_NAME} mappers storages)

# mosaic.deps is needed because the mongo mosaic cache storage needs the mosaic cache
target_link_libraries(${PLUGIN_BASE_NAME}.deps catapult.plugins.mosaic.deps)
