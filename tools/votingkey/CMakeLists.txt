cmake_minimum_required(VERSION 3.13)

catapult_define_tool(votingkey)
target_link_libraries(catapult.tools.votingkey catapult.crypto_voting catapult.finalization)
