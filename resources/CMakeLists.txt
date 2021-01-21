cmake_minimum_required(VERSION 3.13)

set(TARGET_NAME resources)
file(GLOB ${TARGET_NAME}_FILES "*.properties" "*.json")
foreach(_it ${${TARGET_NAME}_FILES})
	set_source_files_properties(${_it} PROPERTIES HEADER_FILE_ONLY TRUE)
endforeach()

source_group("data" FILES ${${TARGET_NAME}_FILES})
add_custom_target(${TARGET_NAME} SOURCES ${${TARGET_NAME}_FILES})
