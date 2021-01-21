cmake_minimum_required(VERSION 3.13)

catapult_test_executable_target(tests.catapult.ionet net)
catapult_add_openssl_dependencies(tests.catapult.ionet)
