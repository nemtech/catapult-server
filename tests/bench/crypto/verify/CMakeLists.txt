cmake_minimum_required(VERSION 3.13)

catapult_bench_executable_target(bench.catapult.crypto.verify)
target_link_libraries(bench.catapult.crypto.verify catapult.crypto bench.catapult.bench.nodeps)
