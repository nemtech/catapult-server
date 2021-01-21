cmake_minimum_required(VERSION 3.13)

set(TARGET_NAME catapult.mongo)

catapult_library_target(${TARGET_NAME} mappers storages)
catapult_add_mongo_dependencies(${TARGET_NAME})
# DO NOT alter the order, with model before plugins, linker is not pulling model::CreateNotificationPublisher producing invalid libextension.mongo as a result
# catapult.validators is needed due to validators::AggregateValidationResult
target_link_libraries(${TARGET_NAME} catapult.extensions catapult.plugins catapult.model catapult.validators)
