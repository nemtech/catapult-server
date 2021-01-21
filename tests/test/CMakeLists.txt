cmake_minimum_required(VERSION 3.13)

catapult_add_gtest_dependencies()

add_subdirectory(cache)
add_subdirectory(core)
add_subdirectory(crypto)
add_subdirectory(local)
add_subdirectory(nemesis)
add_subdirectory(net)
add_subdirectory(nodeps)
add_subdirectory(other)
add_subdirectory(plugins)
add_subdirectory(tree)
