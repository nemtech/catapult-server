cmake_minimum_required(VERSION 3.13)

catapult_add_gtest_dependencies()
catapult_library_target(tests.catapult.test.mongo mocks)
catapult_add_mongo_dependencies(tests.catapult.test.mongo)
target_link_libraries(tests.catapult.test.mongo tests.catapult.test.core)
