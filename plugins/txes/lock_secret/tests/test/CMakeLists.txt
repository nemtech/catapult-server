cmake_minimum_required(VERSION 3.13)

catapult_add_gtest_dependencies()
catapult_library_target(tests.catapult.test.plugins.locksecret)
target_link_libraries(tests.catapult.test.plugins.locksecret catapult.plugins.locksecret.deps)
