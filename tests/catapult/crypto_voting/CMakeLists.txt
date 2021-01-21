cmake_minimum_required(VERSION 3.13)

catapult_test_executable_target(tests.catapult.crypto_voting core test)
target_link_libraries(tests.catapult.crypto_voting tests.catapult.test.crypto)
