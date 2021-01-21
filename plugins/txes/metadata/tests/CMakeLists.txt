cmake_minimum_required(VERSION 3.13)

add_subdirectory(test)

set(TARGET_NAME tests.catapult.plugins.metadata)

catapult_tx_plugin_tests(${TARGET_NAME})
