cmake_minimum_required(VERSION 3.13)

catapult_define_extension_src(partialtransaction api chain handlers)
target_link_libraries(catapult.partialtransaction catapult.consumers catapult.plugins.aggregate.sdk)
