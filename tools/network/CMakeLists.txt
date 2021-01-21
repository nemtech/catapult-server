cmake_minimum_required(VERSION 3.13)

catapult_define_tool(network)
target_link_libraries(catapult.tools.network catapult.nodediscovery)
