cmake_minimum_required(VERSION 3.13)

catapult_library_target(tests.catapult.test.net mocks)
target_link_libraries(tests.catapult.test.net tests.catapult.test.core tests.catapult.test.crypto catapult.net)
catapult_add_openssl_dependencies(tests.catapult.test.net)
