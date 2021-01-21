cmake_minimum_required(VERSION 3.13)

set(PLUGIN_BASE_NAME catapult.mongo.plugins.restrictionmosaic)

include_directories(${PROJECT_SOURCE_DIR}/plugins/txes/restriction_mosaic)
catapult_mongo_plugin_src_with_deps(${PLUGIN_BASE_NAME} mappers storages)

# restrictionmosaic.deps is needed because the mongo mosaic restriction cache storage needs the mosaic restriction cache
target_link_libraries(${PLUGIN_BASE_NAME}.deps catapult.plugins.restrictionmosaic.deps)
