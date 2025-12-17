#######################################################################
# CMake configuration
cmake_minimum_required(VERSION 3.25.1)
cmake_policy(VERSION 3.25.1)

# Adds custom CMake scripts/modules
list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/../../cmake)

#######################################################################
# Declare project and options.
project(SpriteSheetTool VERSION 1.0.0 DESCRIPTION "Sprite Sheet tool. Split or merge sprite animations." LANGUAGES CXX)

set(CMAKE_VERBOSE_MAKEFILE On)
set(CMAKE_EXPORT_COMPILE_COMMANDS On)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/${CMAKE_BUILD_TYPE})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/${CMAKE_BUILD_TYPE})
set_property(GLOBAL PROPERTY USE_FOLDERS On)





###### Project configuration ######

# Root header files
file(GLOB_RECURSE HEADER_FILES ${CMAKE_CURRENT_SOURCE_DIR}/src/*.hpp)

# Root source files
file(GLOB_RECURSE SOURCE_FILES ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp)

add_executable(${PROJECT_NAME} ${SOURCE_FILES})

target_sources(${PROJECT_NAME} PUBLIC FILE_SET HEADERS BASE_DIRS ${CMAKE_CURRENT_SOURCE_DIR} FILES ${HEADER_FILES})

set_target_properties(${PROJECT_NAME} PROPERTIES
	CXX_STANDARD 20
	CXX_STANDARD_REQUIRED On
	CXX_EXTENSIONS Off
)

if ( MSVC )
	target_compile_options(${PROJECT_NAME} PRIVATE
		/wd4100       # Ignore "unreferenced formal parameter" warning
		/wd4127       # Ignore "conditional expression is constant" warning
		/wd4244       # Ignore "conversion possible loss of data" warning
		/wd4324       # Ignore "structure was padded due to alignment specifier" warning
		/wd4481       # Ignore "nonstandard extension used: override" warning
		/wd4512       # Ignore "assignment operator could not be generated" warning
		/wd4701       # Ignore "potentially uninitialized local variable" warning
		/wd4702       # Ignore "unreachable code" warning
		/wd4996       # Ignore "function or variable may be unsafe" warning
		/MP${CMAKE_BUILD_PARALLEL_LEVEL} # Multiprocess compilation
		/Gy           # Enable function-level linking
		/GR-          # Disable run-time type information
		/EHsc		  # Enable exceptions
		/W4           # Warning level 4
		/Ob2
		$<$<CONFIG:Debug>:
			-DDEBUG
			/RTC1         # Disable optimizations
			/Od           # Enable basic run-time checks
			#/WX           # Treat warnings as errors
			/Zi
		>
		$<$<CONFIG:Release>:
			-DNDEBUG
			/O2           # Optimize for maximum speed
			/Ob2          # Inline any suitable function
			/GF           # Enable string pooling
		>
	)
else ( UNIX )
	target_compile_options(${PROJECT_NAME} PRIVATE
		-Wno-unknown-pragmas
		-Wno-unused
		-fno-exceptions
		-fno-rtti
		$<$<CONFIG:Debug>:
			-O0
			-g
			-Wall
			-Wextra
			-Wpedantic
			-Werror
		>
		$<$<CONFIG:Release>:
			-O2
			-ffast-math
			-fdata-sections
			-ffunction-sections
		>
	)
endif ()

target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)



###### Sub-Projects ######

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../.. Emeraude)

target_include_directories(${PROJECT_NAME} PUBLIC ${EMERAUDE_SOURCE_DIR})
target_link_libraries(${PROJECT_NAME} PUBLIC Emeraude)
