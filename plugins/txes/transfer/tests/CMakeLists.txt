cmake_minimum_required(VERSION 3.13)

set(TARGET_NAME tests.catapult.plugins.transfer)

catapult_tx_plugin_tests(${TARGET_NAME})
target_link_libraries(tests.catapult.plugins.transfer tests.catapult.test.net)
