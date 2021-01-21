cmake_minimum_required(VERSION 3.13)

catapult_library_target(bench.catapult.bench.nodeps)
catapult_add_benchmark_dependencies(bench.catapult.bench.nodeps)
target_link_libraries(bench.catapult.bench.nodeps catapult.utils catapult.version)
