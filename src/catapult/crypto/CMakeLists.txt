cmake_minimum_required(VERSION 3.13)

include_directories(../../../external)

catapult_library_target(catapult.crypto)

target_link_libraries(catapult.crypto catapult.utils external)
catapult_add_openssl_dependencies(catapult.crypto)
