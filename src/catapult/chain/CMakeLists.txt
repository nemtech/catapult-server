cmake_minimum_required(VERSION 3.13)

catapult_library_target(catapult.chain)
target_link_libraries(catapult.chain
	catapult.api
	catapult.cache_core
	catapult.cache_tx
	catapult.disruptor
	catapult.model
	catapult.observers
	catapult.utils
	catapult.validators)
