cmake_minimum_required(VERSION 3.13)

set(PLUGIN_BASE_NAME catapult.mongo.plugins.multisig)

include_directories(${PROJECT_SOURCE_DIR}/plugins/txes/multisig)
catapult_mongo_plugin_src_with_deps(${PLUGIN_BASE_NAME} mappers storages)
