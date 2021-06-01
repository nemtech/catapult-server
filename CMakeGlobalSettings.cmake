### enable testing
enable_testing()

### enable ccache if available
find_program(CCACHE_FOUND ccache)
if(CCACHE_FOUND)
	set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE ccache)
	set_property(GLOBAL PROPERTY RULE_LAUNCH_LINK ccache)
endif(CCACHE_FOUND)

### set general cmake settings
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

### set up conan
if(USE_CONAN)
	set(CONAN_SYSTEM_INCLUDES ON)
	include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)

	if(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
		conan_basic_setup(KEEP_RPATHS)
	else()
		conan_basic_setup()
	endif()
endif()

### set boost settings
add_definitions(-DBOOST_ALL_DYN_LINK)
add_definitions(-DBOOST_ASIO_USE_TS_EXECUTOR_AS_DEFAULT)
set(Boost_USE_STATIC_LIBS OFF)
set(Boost_USE_MULTITHREADED ON)
set(Boost_USE_STATIC_RUNTIME OFF)

# log requires { atomic chrono filesystem log_setup regex thread }
set(CATAPULT_BOOST_COMPONENTS atomic chrono date_time filesystem log log_setup program_options regex thread)

### set openssl definitions
add_definitions(-DOPENSSL_API_COMPAT=0x10100000L)

### set custom diagnostics
if(ENABLE_CATAPULT_DIAGNOSTICS)
	add_definitions(-DENABLE_CATAPULT_DIAGNOSTICS)
endif()

### forward docker build settings
if(CATAPULT_TEST_DB_URL)
	add_definitions(-DCATAPULT_TEST_DB_URL="${CATAPULT_TEST_DB_URL}")
endif()
if(CATAPULT_DOCKER_TESTS)
	add_definitions(-DCATAPULT_DOCKER_TESTS)
endif()

### set architecture
if(ARCHITECTURE_NAME)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -march=${ARCHITECTURE_NAME}")
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -march=${ARCHITECTURE_NAME}")
endif()

### set code coverage
if(ENABLE_CODE_COVERAGE)
	if("${CMAKE_CXX_COMPILER_ID}" MATCHES "GNU")
		set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} --coverage -fprofile-arcs -ftest-coverage")
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} --coverage -fprofile-arcs -ftest-coverage")
	elseif("${CMAKE_CXX_COMPILER_ID}" MATCHES "Clang")
		set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fprofile-instr-generate -fcoverage-mapping")
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fprofile-instr-generate -fcoverage-mapping")
	else()
		message(FATAL_ERROR "code coverage is unsupported for ${CMAKE_CXX_COMPILER_ID}")
	endif()
endif()

### set sanitization
if(ENABLE_FUZZ_BUILD)
	set(USE_SANITIZER "undefined")
endif()

if(USE_SANITIZER)
	set(SANITIZER_BLACKLIST "${PROJECT_SOURCE_DIR}/sanitizer_blacklist.txt")
	set(SANITIZATION_FLAGS "-fno-omit-frame-pointer -fsanitize-blacklist=${SANITIZER_BLACKLIST} -fsanitize=${USE_SANITIZER}")

	if(USE_SANITIZER MATCHES "undefined")
		set(SANITIZATION_FLAGS "${SANITIZATION_FLAGS} -fsanitize=implicit-conversion,nullability")
		if(ENABLE_FUZZ_BUILD)
			set(SANITIZATION_FLAGS "${SANITIZATION_FLAGS} -fsanitize=address -fno-sanitize-recover=all")
		endif()
	endif()

	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${SANITIZATION_FLAGS}")
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${SANITIZATION_FLAGS}")
endif()

### set compiler settings
if(MSVC)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W4 /WX /EHsc")
	# in debug disable "potentially uninitialized local variable" (FP)
	set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /MDd /D_SCL_SECURE_NO_WARNINGS /wd4701")
	set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO} /MD /Zi")
	set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /MD")

	set(CMAKE_EXE_LINKER_FLAGS_DEBUG "${CMAKE_EXE_LINKER_FLAGS_DEBUG} /DEBUG:FASTLINK")
	set(CMAKE_EXE_LINKER_FLAGS_RELWITHDEBINFO "${CMAKE_EXE_LINKER_FLAGS_RELWITHDEBINFO} /DEBUG")

	add_definitions(-D_WIN32_WINNT=0x0601 /w44287 /w44388)

	# explicitly disable linking against static boost libs
	add_definitions(-DBOOST_ALL_NO_LIB)

	# min/max macros are useless
	add_definitions(-DNOMINMAX)
	add_definitions(-DWIN32_LEAN_AND_MEAN)

	# mongo cxx view inherits std::iterator
	add_definitions(-D_SILENCE_CXX17_ITERATOR_BASE_CLASS_DEPRECATION_WARNING)
	# boost asio associated_allocator
	add_definitions(-D_SILENCE_CXX17_ALLOCATOR_VOID_DEPRECATION_WARNING)
elseif("${CMAKE_CXX_COMPILER_ID}" MATCHES "GNU")
	# -Wstrict-aliasing=1 perform most paranoid strict aliasing checks
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wpedantic -Wshadow -Wconversion -Wformat-security -Werror -Wstrict-aliasing=1")

	# - Wno-maybe-uninitialized: false positives where gcc isn't sure if an uninitialized variable is used or not
	set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO} -Wno-maybe-uninitialized -g1 -fno-omit-frame-pointer")
	set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -Wno-maybe-uninitialized")

	# add memset_s
	add_definitions(-D_STDC_WANT_LIB_EXT1_=1)
	add_definitions(-D__STDC_WANT_LIB_EXT1__=1)
elseif("${CMAKE_CXX_COMPILER_ID}" MATCHES "Clang")
	# - Wno-c++98-compat*: catapult is not compatible with C++98
	# - Wno-disabled-macro-expansion: expansion of recursive macro is required for boost logging macros
	# - Wno-padded: allow compiler to automatically pad data types for alignment
	# - Wno-switch-enum: do not require enum switch statements to list every value (this setting is also incompatible with GCC warnings)
	# - Wno-weak-vtables: vtables are emitted in all translation units for virtual classes with no out-of-line virtual method definitions
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} \
		-stdlib=libc++ \
		-Weverything \
		-Werror \
		-Wno-c++98-compat \
		-Wno-c++98-compat-pedantic \
		-Wno-disabled-macro-expansion \
		-Wno-padded \
		-Wno-switch-enum \
		-Wno-weak-vtables")

	set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO} -g1")
endif()

if(NOT MSVC)
	# set visibility flags
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fvisibility=hidden")
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fvisibility=hidden")
endif()

if(CATAPULT_BUILD_RELEASE)
	set(ENABLE_HARDENING ON)
endif()

if("${CMAKE_SYSTEM_NAME}" MATCHES "Linux")
	# set hardening flags
	if(ENABLE_HARDENING)
		set(HARDENING_FLAGS "-fstack-protector-all -D_FORTIFY_SOURCE=2")
		if("${CMAKE_CXX_COMPILER_ID}" MATCHES "GNU")
			set(HARDENING_FLAGS "${HARDENING_FLAGS} -fstack-clash-protection")
		else()
			set(HARDENING_FLAGS "${HARDENING_FLAGS} -fsanitize=safe-stack")
		endif()

		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${HARDENING_FLAGS}")
		set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${HARDENING_FLAGS}")

		set(LINKER_HARDENING_FLAGS "-Wl,-z,noexecstack,-z,relro,-z,now")
		set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} ${LINKER_HARDENING_FLAGS}")
		set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${LINKER_HARDENING_FLAGS}")
	endif()
endif()

if(USE_CONAN)
	# only set rpath when running conan, which copies dependencies to `@executable_path/../deps`
	# when not using conan, rpath is set to link paths by default
	if(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
		set(ENABLE_RPATHS ON)
		set(USE_EXPLICIT_RPATHS ON)
	endif()
endif()

if(ENABLE_RPATHS)
	if(USE_EXPLICIT_RPATHS)
		if("${CMAKE_SYSTEM_NAME}" MATCHES "Linux")
			# $origin - to load plugins when running the server
			set(CMAKE_INSTALL_RPATH "$ORIGIN/../deps:$ORIGIN/../lib")
			set(CMAKE_BUILD_WITH_INSTALL_RPATH TRUE)
			set(CMAKE_INSTALL_RPATH_USE_LINK_PATH FALSE)

			# use rpath for executables
			# (executable rpath will be used for loading indirect libs, this is needed because boost libs do not set runpath)
			# use newer runpath for shared libs
			set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -Wl,--enable-new-dtags")
			set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,--disable-new-dtags")
		endif()
		if(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
			set(CMAKE_INSTALL_RPATH "@executable_path/../deps;@executable_path/../lib")
			set(CMAKE_BUILD_WITH_INSTALL_RPATH TRUE)
			set(CMAKE_INSTALL_RPATH_USE_LINK_PATH FALSE)
		endif()
	endif()
else()
	set(CMAKE_SKIP_BUILD_RPATH TRUE)
endif()

### define gtest helper functions

if(ENABLE_TESTS)
	find_package(GTest 1.10.0 EXACT REQUIRED)
endif()

# find and set gtest includes
function(catapult_add_gtest_dependencies)
	include_directories(SYSTEM ${GTEST_INCLUDE_DIR})
endfunction()

# add tests subdirectory
function(catapult_add_tests_subdirectory DIRECTORY_NAME)
	if(ENABLE_TESTS)
		add_subdirectory(${DIRECTORY_NAME})
	endif()
endfunction()

# sets additional compiler options for test projects in order to quiet GTEST warnings while allowing source warning checks to be stricter
function(catapult_set_test_compiler_options)
	# some gtest workarounds for gcc + clang
	if("${CMAKE_CXX_COMPILER_ID}" MATCHES "GNU")
		# - Wno-dangling-else: workaround for GTEST ambiguous else blocker not working https://github.com/google/googletest/issues/1119
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} \
			-Wno-dangling-else"
			PARENT_SCOPE)
	elseif("${CMAKE_CXX_COMPILER_ID}" MATCHES "Clang")
		# - Wno-global-constructors: required for GTEST test definition macros
		# - Wno-zero-as-null-pointer-constant: workaround for GTEST NULL/nullptr mismatch https://github.com/google/googletest/issues/1323
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} \
			-Wno-global-constructors \
			-Wno-zero-as-null-pointer-constant"
			PARENT_SCOPE)
	endif()
endfunction()

### define version helpers

# set CATAPULT_VERSION_DESCRIPTION to a reasonable value
if(NOT CATAPULT_BUILD_DEVELOPMENT)
	# extract version information from git
	execute_process(
		COMMAND git rev-parse --abbrev-ref HEAD
		WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
		OUTPUT_VARIABLE GIT_BRANCH
		OUTPUT_STRIP_TRAILING_WHITESPACE)
	execute_process(
		COMMAND git log -1 --format=%h
		WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
		OUTPUT_VARIABLE GIT_COMMIT_HASH
		OUTPUT_STRIP_TRAILING_WHITESPACE)

	if(CATAPULT_BUILD_RELEASE_PUBLIC)
		set(CATAPULT_VERSION_DESCRIPTION "(public)")
	else()
		set(CATAPULT_VERSION_DESCRIPTION "${GIT_COMMIT_HASH} [${GIT_BRANCH}]")
	endif()
endif()

if(MSVC)
	function(set_win_version_definitions WIN_TARGET_NAME WIN_FILETYPE)
		add_definitions(-DCATAPULT_VERSION_DESCRIPTION="${CATAPULT_VERSION_DESCRIPTION}")
		add_definitions(-DWIN_TARGET_NAME=${WIN_TARGET_NAME})
		add_definitions(-DWIN_FILETYPE=${WIN_FILETYPE})

		if(CATAPULT_BUILD_RELEASE)
			add_definitions(-DCATAPULT_BUILD_RELEASE=1)
		endif()
	endfunction()

	# embed the version rc file
	set(VERSION_RESOURCES ${CMAKE_SOURCE_DIR}/src/catapult/version/win/win_version.rc)
else()
	# since strings are not referenced, in order for the linker to include them, they must be forcibly linked via an object file
	set(VERSION_RESOURCES $<TARGET_OBJECTS:catapult.version.nix>)
endif()

### define target helper functions

# sets cxx std version
function(catapult_set_cxx_std_version TARGET_NAME)
	set_property(TARGET ${TARGET_NAME} PROPERTY CXX_STANDARD 17)
	if("${CMAKE_CXX_COMPILER_ID}" MATCHES "GNU" AND "${CMAKE_CXX_COMPILER_VERSION}" MATCHES "^8.")
		target_link_libraries(${TARGET_NAME} "stdc++fs")
	endif()
endfunction()

# used to define a catapult target (library, executable) and automatically enables PCH for clang
function(catapult_target TARGET_NAME)
	catapult_set_cxx_std_version(${TARGET_NAME})

	# indicate boost as a dependency
	target_link_libraries(${TARGET_NAME} ${Boost_LIBRARIES})

	# put both plugins and plugins tests in same 'folder'
	if(TARGET_NAME MATCHES "\.plugins")
		set_property(TARGET ${TARGET_NAME} PROPERTY FOLDER "plugins")
	endif()

	if(TARGET_NAME MATCHES "\.tools")
		set_property(TARGET ${TARGET_NAME} PROPERTY FOLDER "tools")
	endif()
endfunction()

# finds all files comprising a target
function(catapult_find_all_target_files TARGET_TYPE TARGET_NAME)
	if(CMAKE_VERBOSE_MAKEFILE)
		message("processing ${TARGET_TYPE} '${TARGET_NAME}'")
	endif()

	file(GLOB ${TARGET_NAME}_INCLUDE_SRC "*.h")
	file(GLOB ${TARGET_NAME}_SRC "*.cpp")

	set(CURRENT_FILES ${${TARGET_NAME}_INCLUDE_SRC} ${${TARGET_NAME}_SRC})
	SOURCE_GROUP("src" FILES ${CURRENT_FILES})
	set(TARGET_FILES ${CURRENT_FILES})

	# add any (optional) subdirectories
	foreach(arg ${ARGN})
		set(SUBDIR ${arg})
		if(CMAKE_VERBOSE_MAKEFILE)
			message("+ processing subdirectory '${arg}'")
		endif()

		file(GLOB ${TARGET_NAME}_${SUBDIR}_INCLUDE_SRC "${SUBDIR}/*.h")
		file(GLOB ${TARGET_NAME}_${SUBDIR}_SRC "${SUBDIR}/*.cpp")

		set(CURRENT_FILES ${${TARGET_NAME}_${SUBDIR}_INCLUDE_SRC} ${${TARGET_NAME}_${SUBDIR}_SRC})
		SOURCE_GROUP("${SUBDIR}" FILES ${CURRENT_FILES})
		set(TARGET_FILES ${TARGET_FILES} ${CURRENT_FILES})
	endforeach()

	set(${TARGET_NAME}_FILES ${TARGET_FILES} PARENT_SCOPE)
endfunction()

# used to define a catapult object library
function(catapult_object_library TARGET_NAME)
	add_library(${TARGET_NAME} OBJECT ${ARGN})
	set_property(TARGET ${TARGET_NAME} PROPERTY POSITION_INDEPENDENT_CODE ON)

	catapult_set_cxx_std_version(${TARGET_NAME})
endfunction()

# used to define a catapult library, creating an appropriate source group and adding a library
function(catapult_library TARGET_NAME)
	catapult_find_all_target_files("lib" ${TARGET_NAME} ${ARGN})
	add_library(${TARGET_NAME} ${${TARGET_NAME}_FILES})
endfunction()

# combines catapult_library and catapult_target
function(catapult_library_target TARGET_NAME)
	catapult_library(${TARGET_NAME} ${ARGN})
	set_property(TARGET ${TARGET_NAME} PROPERTY POSITION_INDEPENDENT_CODE ON)
	catapult_target(${TARGET_NAME})
endfunction()

# used to define a catapult shared library, creating an appropriate source group and adding a library
function(catapult_shared_library TARGET_NAME)
	catapult_find_all_target_files("shared lib" ${TARGET_NAME} ${ARGN})

	add_definitions(-DDLL_EXPORTS)

	if(MSVC)
		set_win_version_definitions(${TARGET_NAME} VFT_DLL)
	endif()

	add_library(${TARGET_NAME} SHARED ${${TARGET_NAME}_FILES} ${VERSION_RESOURCES})
endfunction()

# combines catapult_shared_library and catapult_target
function(catapult_shared_library_target TARGET_NAME)
	catapult_shared_library(${TARGET_NAME} ${ARGN})
	catapult_target(${TARGET_NAME})

	install(TARGETS ${TARGET_NAME})
endfunction()

# used to define a catapult executable, creating an appropriate source group and adding an executable
function(catapult_executable TARGET_NAME)
	catapult_find_all_target_files("exe" ${TARGET_NAME} ${ARGN})

	if(MSVC)
		set_win_version_definitions(${TARGET_NAME} VFT_APP)
	endif()

	add_executable(${TARGET_NAME} ${${TARGET_NAME}_FILES} ${VERSION_RESOURCES})

	if(WIN32 AND MINGW)
		target_link_libraries(${TARGET_NAME} wsock32 ws2_32)
	endif()
endfunction()

# used to define a catapult header only target, creating an appropriate source group in order to allow VS to create an appropriate folder
function(catapult_header_only_target TARGET_NAME)
	if(MSVC)
		catapult_find_all_target_files("hdr" ${TARGET_NAME} ${ARGN})

		if(CMAKE_VERBOSE_MAKEFILE)
			foreach(arg ${ARGN})
				message("adding subdirectory '${arg}'")
			endforeach()
		endif()

		# https://stackoverflow.com/questions/39887352/how-to-create-a-cmake-header-only-library-that-depends-on-external-header-files
		# target_sources doesn't work with interface libraries, but we can use custom_target (with empty action)
		add_custom_target(${TARGET_NAME} SOURCES ${${TARGET_NAME}_FILES})
	endif()
endfunction()

# used to define a catapult test executable
function(catapult_test_executable TARGET_NAME)
	include_directories(SYSTEM ${GTEST_INCLUDE_DIR})

	catapult_executable(${TARGET_NAME} ${ARGN})
	add_test(NAME ${TARGET_NAME} WORKING_DIRECTORY ${CMAKE_BINARY_DIR} COMMAND ${TARGET_NAME})

	target_link_libraries(${TARGET_NAME} ${GTEST_LIBRARIES})
endfunction()

# used to define a catapult test executable for a catapult library by combining catapult_test_executable and
# catapult_target and adding some library dependencies
function(catapult_test_executable_target TARGET_NAME TEST_DEPENDENCY_NAME)
	catapult_test_executable(${TARGET_NAME} ${ARGN})

	# inline instead of calling catapult_add_gtest_dependencies in order to apply gtest dependencies to correct scope
	include_directories(SYSTEM ${GTEST_INCLUDE_DIR})

	# customize and export compiler options for gtest
	catapult_set_test_compiler_options()
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}" PARENT_SCOPE)

	# test libraries are in the form test.xyz, so add xyz as a dependency (the library under test)
	string(FIND ${TARGET_NAME} "." TEST_END_INDEX)
	MATH(EXPR TEST_END_INDEX "${TEST_END_INDEX}+1")
	string(SUBSTRING ${TARGET_NAME} ${TEST_END_INDEX} -1 LIBRARY_UNDER_TEST)

	target_link_libraries(${TARGET_NAME} tests.catapult.test.${TEST_DEPENDENCY_NAME} ${LIBRARY_UNDER_TEST})
	catapult_target(${TARGET_NAME})
endfunction()

# used to define a catapult test executable for a header only catapult library by combining catapult_test_executable and
# catapult_target and adding some library dependencies
function(catapult_test_executable_target_header_only TARGET_NAME TEST_DEPENDENCY_NAME)
	catapult_test_executable(${TARGET_NAME} ${ARGN})

	# customize and export compiler options for gtest
	catapult_set_test_compiler_options()
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}" PARENT_SCOPE)

	target_link_libraries(${TARGET_NAME} tests.catapult.test.${TEST_DEPENDENCY_NAME})
	catapult_target(${TARGET_NAME})
endfunction()

# used to define a catapult tool executable
function(catapult_define_tool TOOL_NAME)
	set(TARGET_NAME catapult.tools.${TOOL_NAME})

	catapult_executable(${TARGET_NAME})
	target_link_libraries(${TARGET_NAME} catapult.tools)
	catapult_target(${TARGET_NAME})

	add_dependencies(tools ${TARGET_NAME})

	install(TARGETS ${TARGET_NAME})
endfunction()
