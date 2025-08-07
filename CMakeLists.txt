#######################################################################
# CMake configuration
cmake_minimum_required(VERSION 3.25.1)
cmake_policy(SET CMP0042 NEW) # MACOSX_RPATH is enabled by default.
cmake_policy(SET CMP0091 NEW) # MSVC runtime library flags are selected by an abstraction.
#cmake_policy(SET CMP0177 NEW) # Normalize all install path 3.31

# Adds custom CMake scripts/modules
list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

message("Using CMake ${CMAKE_VERSION}")

#######################################################################
# Configure the parallel job count
cmake_host_system_information(RESULT DETECTED_JOB_COUNT QUERY NUMBER_OF_PHYSICAL_CORES)

set(JOB_COUNT ${DETECTED_JOB_COUNT} CACHE STRING "The number of job used for compilation. Default auto.")
set(CMAKE_BUILD_PARALLEL_LEVEL ${JOB_COUNT})

message("Job processes used : ${CMAKE_BUILD_PARALLEL_LEVEL}")

#######################################################################
# Declare project and options.
project(
	Emeraude
	VERSION 0.7.5
	DESCRIPTION "Emeraude Engine is an open-source (LGPLv3) 3D Graphics engine using Vulkan and written in C++20."
	HOMEPAGE_URL "https://github.com/londnoir/emeraude-engine"
	LANGUAGES C CXX
)

set(CMAKE_VERBOSE_MAKEFILE On)
set(CMAKE_EXPORT_COMPILE_COMMANDS On)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/${CMAKE_BUILD_TYPE})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/${CMAKE_BUILD_TYPE})

set_property(GLOBAL PROPERTY USE_FOLDERS On)

# Emeraude libraries project variables and options
set(EMERAUDE_ENABLE_TESTS Off CACHE BOOL "Enable the Emeraude testing suite (Default Off).")
set(EMERAUDE_INTERNET_CHECK_DOMAIN "google.com" CACHE STRING "Web domain to check internet connexion (Default google.com).")
set(EMERAUDE_COMPILATION_DIR "" CACHE STRING "Declare where the project is compiled. For macOS this should point the 'package.app' in the compilation folder.")
set(EMERAUDE_USE_SYSTEM_LIBS Off CACHE BOOL "Use the system for common libraries instead of the embedded ones (Default Off).")
set(EMERAUDE_ENABLE_PCH Off CACHE BOOL "Enable 'PreCompiled Headers' feature to speed up the compilation (Default Off).")
set(EMERAUDE_ENABLE_OPENMP On CACHE BOOL "Enable 'libomp' library [SYSTEM] (Default On).")
set(EMERAUDE_ENABLE_SNDFILE On CACHE BOOL "Enable 'libsndfile' library. Adds the ability to read a wide range of sound formats [SYSTEM] (Default On).")
set(EMERAUDE_ENABLE_SAMPLERATE On CACHE BOOL "Enable 'libsamplerate' library. Adds the ability to resample a sound on the fly to meet OpenAL settings [SYSTEM|LOCAL] (Default On).")
set(EMERAUDE_ENABLE_ASIO On CACHE BOOL "Enable 'libasio' library. Adds the networking ability [SYSTEM|LOCAL] (Default On).")
set(EMERAUDE_ENABLE_CPUFEATURES On CACHE BOOL "Enable 'cpu_features' library. Adds the cpu identification ability [LOCAL] (Default On).")
set(EMERAUDE_ENABLE_HWLOC On CACHE BOOL "Enable 'HWLOC' library. Adds the cpu cores/threads info ability [SYSTEM|LOCAL] (Default On).")
set(EMERAUDE_ENABLE_OPENAL On CACHE BOOL "Enable 'libopenal' library. Adds the audio to the engine [LOCAL] (Default On).")
set(EMERAUDE_ENABLE_TAGLIB On CACHE BOOL "Enable 'libtaglib' library. Adds the ability to read media file tags [SYSTEM] (Default On).")
set(EMERAUDE_ENABLE_IMGUI Off CACHE BOOL "Enable 'libimgui' library. Adds a GUI to the engine [LOCAL] (Default Off).")
set(EMERAUDE_ENABLE_BULLET Off CACHE BOOL "Enable 'libBullet' library. Adds the Bullet physics engine [SYSTEM] (Default Off).")
set(EMERAUDE_ENABLE_FASTGLTF Off CACHE BOOL "Enable 'fastgltf' library. Adds a glTF file format reader [SYSTEM] (Default Off).")
set(EMERAUDE_ENABLE_TSAN Off CACHE BOOL "Enable 'ThreadSanitizer' library. Adds a dev tool to trace data race [SYSTEM] (Linux only, Default Off).")
# Debug preprocessor macros control (Ignored in Release).
option(EMERAUDE_DEBUG_OBSERVER_PATTERN "Enable the debug output of observer pattern (Default Off)." Off)
option(EMERAUDE_DEBUG_PIXEL_FACTORY "Enable the debug output of pixel factory library (Default Off)." Off)
option(EMERAUDE_DEBUG_VERTEX_FACTORY "Enable the debug output of vertex factory library (Default Off)." Off)
option(EMERAUDE_DEBUG_WAVE_FACTORY "Enable the debug output of wave factory library (Default Off)." Off)
option(EMERAUDE_DEBUG_KEYBOARD_INPUT "Enable printing of events to the console from keyboard inputs (Default Off)." Off)
option(EMERAUDE_DEBUG_POINTER_INPUT "Enable printing of events to the console from pointer inputs (Default Off)." Off)
option(EMERAUDE_DEBUG_WINDOW_EVENTS "Enable printing of events to the console from window events (Default Off)." Off)
option(EMERAUDE_DEBUG_VULKAN_TRACKING "Enable printing of Vulkan objects lifespan (Default Off)." Off)
option(EMERAUDE_DEBUG_THREAD_POOL "Enable printing of ThreadPool (Default Off)." Off)

# Force the compilation directory for testing (Only if empty)
if ( EMERAUDE_ENABLE_TESTS AND "${EMERAUDE_COMPILATION_DIR}" STREQUAL "" )
	message(STATUS "EMERAUDE_ENABLE_TESTS has been enabled but the EMERAUDE_COMPILATION_DIR is not set. Forcing to : '${CMAKE_BINARY_DIR}/${CMAKE_BUILD_TYPE}'.")

	set(EMERAUDE_COMPILATION_DIR "${CMAKE_BINARY_DIR}/${CMAKE_BUILD_TYPE}" CACHE STRING "" FORCE)
endif ()

# Create constant variable for dependency inclusions
get_filename_component(TEMP_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src ABSOLUTE)
set(EMERAUDE_SOURCE_DIR ${TEMP_DIR} CACHE STRING "Emeraude-Engine source directory.")

# Create constant variable for dependency inclusions
get_filename_component(TEMP_DIR ${CMAKE_CURRENT_SOURCE_DIR}/dependencies ABSOLUTE)
set(EMERAUDE_DEPENDENCIES_DIR ${TEMP_DIR} CACHE STRING "Emeraude-Engine dependencies directory.")

# Common libraries directory
if ( UNIX AND NOT APPLE )
	message("Linux platform detected !")

	if ( NOT DEFINED APP_ARCH )
		set(APP_ARCH "x86_64")
	endif ()

	set(LOCAL_LIB_DIR ${CMAKE_CURRENT_SOURCE_DIR}/dependencies/linux.${APP_ARCH}-${CMAKE_BUILD_TYPE} CACHE STRING "Final directory for local libraries for Linux.")

	set(CMAKE_GEN_EXTRA_FLAGS -G "Ninja" CACHE STRING "Local libraries compilation extra generation flags for Linux.")
elseif ( APPLE )
	message("Apple platform detected ! macOS architecture : ${CMAKE_OSX_ARCHITECTURES}, default target SDK : ${CMAKE_OSX_DEPLOYMENT_TARGET}")

	if ( "${CMAKE_OSX_ARCHITECTURES}" STREQUAL "x86_64" )
		set(APP_ARCH "x86_64")
		set(APP_OSX_DEPLOYMENT_TARGET "12.0" CACHE STRING "Minimum SDK for macOS.")
	elseif ( "${CMAKE_OSX_ARCHITECTURES}" STREQUAL "arm64" )
		set(APP_ARCH "arm64")
		set(APP_OSX_DEPLOYMENT_TARGET "12.0" CACHE STRING "Minimum SDK for macOS.")
	else ()
		message(FATAL_ERROR "Please specify the architecture for macOS with -DCMAKE_OSX_ARCHITECTURES=x86_64 or -DCMAKE_OSX_ARCHITECTURES=arm64 !")
	endif ()

	# NOTE: TO be sure
	set(CMAKE_OSX_ARCHITECTURES ${APP_ARCH})
	set(CMAKE_OSX_DEPLOYMENT_TARGET ${APP_OSX_DEPLOYMENT_TARGET})

	message("macOS target arch ${CMAKE_OSX_ARCHITECTURES} with SDK ${CMAKE_OSX_DEPLOYMENT_TARGET}")

	set(LOCAL_LIB_DIR ${CMAKE_CURRENT_SOURCE_DIR}/dependencies/mac.${APP_ARCH}-${CMAKE_BUILD_TYPE} CACHE STRING "Final directory for local libraries for macOS.")

	set(CMAKE_GEN_EXTRA_FLAGS -DCMAKE_OSX_ARCHITECTURES=${APP_ARCH} -DCMAKE_OSX_DEPLOYMENT_TARGET=${APP_OSX_DEPLOYMENT_TARGET} -G "Ninja" CACHE STRING "Local libraries compilation extra generation flags for macOS.")
elseif ( MSVC OR MSYS OR MINGW )
	message("Windows platform detected !")

	# FIXME: Pass these info directly to MSVC via flags
	set(CMAKE_C_STANDARD 17)
	set(CMAKE_C_STANDARD_REQUIRED On)
	set(CMAKE_C_EXTENSIONS On)
	set(CMAKE_C_FLAGS_DEBUG "-DDEBUG")
	set(CMAKE_C_FLAGS_RELEASE "-DNDEBUG")
	set(CMAKE_CXX_STANDARD 20)
	set(CMAKE_CXX_STANDARD_REQUIRED On)
	set(CMAKE_CXX_EXTENSIONS On)
	set(CMAKE_CXX_FLAGS_DEBUG "-DDEBUG")
	set(CMAKE_CXX_FLAGS_RELEASE "-DNDEBUG")

	set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

	if ( ${APP_ARCH} STREQUAL "" )
		set(APP_ARCH "x86_64")
	endif ()

	set(LOCAL_LIB_DIR ${CMAKE_CURRENT_SOURCE_DIR}/dependencies/windows.${APP_ARCH}-${CMAKE_BUILD_TYPE} CACHE STRING "Final directory for local libraries for Windows.")

	set(CMAKE_GEN_EXTRA_FLAGS -G "Visual Studio 17 2022" -A x64 CACHE STRING "Local libraries compilation extra generation flags for Windows.")
endif ()

# Print out project paths
message("[${PROJECT_NAME}] Project directory : ${CMAKE_SOURCE_DIR}, current : ${CMAKE_CURRENT_SOURCE_DIR}")
message("[${PROJECT_NAME}] Source directory : ${EMERAUDE_SOURCE_DIR}")
message("[${PROJECT_NAME}] Binary directory : ${CMAKE_BINARY_DIR}, current : ${CMAKE_CURRENT_BINARY_DIR}")



#######################################################################
# Pre sources assembly operations.
if ( EMERAUDE_ENABLE_IMGUI )
	include(SetupPreSourcesImGui)
endif ()

include(SetupPreSourcesJsonCpp)


#######################################################################
# Project header and source files.
file(GLOB EMERAUDE_HEADER_FILES ${CMAKE_CURRENT_SOURCE_DIR}/src/*.hpp)
file(GLOB EMERAUDE_SOURCE_FILES ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp)

set(
	EMERAUDE_DIRECTORIES
	${CMAKE_CURRENT_SOURCE_DIR}/src/Libs
	${CMAKE_CURRENT_SOURCE_DIR}/src/Animations
	${CMAKE_CURRENT_SOURCE_DIR}/src/Audio
	${CMAKE_CURRENT_SOURCE_DIR}/src/Console
	${CMAKE_CURRENT_SOURCE_DIR}/src/Graphics
	${CMAKE_CURRENT_SOURCE_DIR}/src/Input
	${CMAKE_CURRENT_SOURCE_DIR}/src/AVConsole
	${CMAKE_CURRENT_SOURCE_DIR}/src/UserDialogs
	${CMAKE_CURRENT_SOURCE_DIR}/src/Overlay
	${CMAKE_CURRENT_SOURCE_DIR}/src/Net
	${CMAKE_CURRENT_SOURCE_DIR}/src/Physics
	${CMAKE_CURRENT_SOURCE_DIR}/src/PlatformSpecific
	${CMAKE_CURRENT_SOURCE_DIR}/src/Resources
	${CMAKE_CURRENT_SOURCE_DIR}/src/Saphir
	${CMAKE_CURRENT_SOURCE_DIR}/src/Scenes
	${CMAKE_CURRENT_SOURCE_DIR}/src/Tool
	${CMAKE_CURRENT_SOURCE_DIR}/src/Vulkan
)

foreach ( EMERAUDE_DIRECTORY ${EMERAUDE_DIRECTORIES} )
	file(GLOB_RECURSE EMERAUDE_DIRECTORY_HEADER_FILES ${EMERAUDE_DIRECTORY}/*.hpp)
	list(APPEND EMERAUDE_HEADER_FILES ${EMERAUDE_DIRECTORY_HEADER_FILES})

	file(GLOB_RECURSE EMERAUDE_DIRECTORY_SOURCE_FILES ${EMERAUDE_DIRECTORY}/*.cpp)
	list(APPEND EMERAUDE_SOURCE_FILES ${EMERAUDE_DIRECTORY_SOURCE_FILES})
endforeach ()

# ImGui library source files
if ( EMERAUDE_ENABLE_IMGUI )
	list(APPEND EMERAUDE_SOURCE_FILES ${IMGUI_SOURCE_FILES})
endif ()

# JsonCpp library source files
list(APPEND EMERAUDE_SOURCE_FILES ${JSONCPP_SOURCE_FILES})

# Dialogs source files
if ( APPLE )
	list(APPEND EMERAUDE_SOURCE_FILES
		${CMAKE_CURRENT_SOURCE_DIR}/src/PlatformSpecific/Desktop/Dialog/Message.mac.mm
		${CMAKE_CURRENT_SOURCE_DIR}/src/PlatformSpecific/Desktop/Dialog/OpenFile.mac.mm
		${CMAKE_CURRENT_SOURCE_DIR}/src/PlatformSpecific/Desktop/Dialog/SaveFile.mac.mm
		${CMAKE_CURRENT_SOURCE_DIR}/src/PlatformSpecific/Desktop/Commands.mac.mm
	)
endif ()



#######################################################################
# Declare the library target.
if ( MSVC )
	add_library(${PROJECT_NAME} SHARED STATIC ${EMERAUDE_SOURCE_FILES})
else ()
	add_library(${PROJECT_NAME} SHARED ${EMERAUDE_SOURCE_FILES}
		src/Libs/ThreadPool.hpp
		src/Libs/ThreadPool.cpp
		src/Resources/ContainerInterface.hpp)
endif ()

target_sources(${PROJECT_NAME} PUBLIC FILE_SET HEADERS BASE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/src FILES ${EMERAUDE_HEADER_FILES})

# Precompiled headers
if ( EMERAUDE_ENABLE_PCH )
	target_precompile_headers(${PROJECT_NAME} PUBLIC ${EMERAUDE_HEADER_FILES})
endif ()

target_compile_definitions(${PROJECT_NAME} PRIVATE $<$<CONFIG:Debug>:DEBUG>)
target_compile_definitions(${PROJECT_NAME} PRIVATE $<$<CONFIG:Release>:NDEBUG>)

set_target_properties(${PROJECT_NAME} PROPERTIES
	C_STANDARD 17
	C_STANDARD_REQUIRED On
	C_EXTENSIONS On
	CXX_STANDARD 20
	CXX_STANDARD_REQUIRED On
	CXX_EXTENSIONS On # "c++20" -> "gnu++20"
)

if ( APPLE )
	set_target_properties(${PROJECT_NAME} PROPERTIES
		FRAMEWORK TRUE
		FRAMEWORK_VERSION ${PROJECT_VERSION}
		MACOSX_FRAMEWORK_IDENTIFIER org.ln-isle.emeraude
		INSTALL_NAME_DIR "@rpath"
		MACOSX_RPATH TRUE
	)
else ()
	set_target_properties(${PROJECT_NAME} PROPERTIES VERSION ${PROJECT_VERSION} SOVERSION ${PROJECT_VERSION_MAJOR})
endif ()



if ( MSVC )
	message("Configuring C++ compiler (>= MSVC 2022) for Windows ...")

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
		#/GR-          # Disable run-time type information
		/EHsc		  # Enable exceptions
		/W4           # Warning level 4
		/Ob2
		$<$<CONFIG:Debug>:-DDEBUG
			/RTC1         # Disable optimizations
			/Od           # Enable basic run-time checks
			#/WX           # Treat warnings as errors
			/Zi
		>
		$<$<CONFIG:Relase>:-DNDEBUG
			/O2           # Optimize for maximum speed
			/Ob2          # Inline any suitable function
			/GF           # Enable string pooling
		>
	)
else ( UNIX )
	if ( APPLE )
		message("Configuring C++ compiler (>= AppleClang 17.0) for macOS ...")

		target_compile_options(${PROJECT_NAME} PRIVATE
			-Wno-unknown-pragmas
			-Wno-unused
			-fno-exceptions
			#-fno-rtti
			-Wno-nullability-extension # Removes warnings from external dependencies libzip.
			#-fexperimental-library # Enables std::jthread from STL.
			$<$<CONFIG:Debug>:
				-O0
				-g
				-Wall
				-Wextra
				-Wpedantic
				#-Werror
			>
			$<$<CONFIG:Relase>:
				-O2
				-ffast-math
				-fdata-sections
				-ffunction-sections
			>
		)
	else ()
		message("Configuring C++ compiler (>= G++13.3) for Linux ...")

		target_compile_options(${PROJECT_NAME} PRIVATE
			-Wno-unknown-pragmas
			-Wno-unused
			-fno-exceptions
			#-fno-rtti
			$<$<CONFIG:Debug>:
				-DDEBUG
				-O0
				-g
				-Wall
				-Wextra
				-Wpedantic
				-Werror
			>
			$<$<CONFIG:Relase>:
				-DNDEBUG
				-O2
				-ffast-math
				-fdata-sections
				-ffunction-sections
			>
		)

		if ( EMERAUDE_ENABLE_TSAN )
			message("Enabling ThreadSanitizer tools ...")

			target_compile_options(${PROJECT_NAME} PRIVATE -fsanitize=thread)
			target_link_options(${PROJECT_NAME} PRIVATE -fsanitize=thread)
		endif ()
	endif ()
endif ()

target_include_directories(${PROJECT_NAME} PUBLIC
    ${CMAKE_BINARY_DIR}/include
    ${LOCAL_LIB_DIR}/include
)

# "portable-file-dialogs" library for LINUX
if ( UNIX AND NOT APPLE )
	target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/dependencies/portable-file-dialogs)
endif ()

target_link_directories(${PROJECT_NAME} PUBLIC ${LOCAL_LIB_DIR}/lib)



###### Dependencies ######

# Magic Enum
target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/dependencies/magic_enum/include)

# Request : brotli, bzip2, harbuzz, png, zlib
include(SetupFreetype)

# Request : zlib bzip2 xz zstd
include(SetupZIP)

# Request : zlib
include(SetupPNG)

# Request : zlib
if ( EMERAUDE_ENABLE_TAGLIB )
	include(SetupTagLib)
endif ()

include(SetupZLIB)

include(SetupBZIP2)

include(SetupLZMA)

include(SetupZSTD)

if ( NOT EMERAUDE_USE_SYSTEM_LIBS )
	include(SetupBrotli)

	include(SetupHarfbuzz)
endif ()

include(SetupJPEG)

include(SetupThreads)

include(SetupPostSourcesJsonCpp)

include(SetupGLFW3)

include(SetupVulkan)

include(SetupReproc)

if ( EMERAUDE_ENABLE_OPENMP AND NOT APPLE )
	include(SetupOpenMP)
endif ()

if ( EMERAUDE_ENABLE_OPENAL )
	include(SetupOpenAL)
endif ()

if ( EMERAUDE_ENABLE_SNDFILE )
	set(BINARY_TARGET_NAME ${PROJECT_NAME})

	include(SetupSndFile)
endif ()

if ( EMERAUDE_ENABLE_SAMPLERATE )
	include(SetupSampleRate)
endif ()

if ( EMERAUDE_ENABLE_ASIO )
	if ( EMERAUDE_ENABLE_TSAN )
		message(WARNING "Disabling ASIO when ThreadSanitizer is requested !")
	else ()
		include(SetupASIO)
	endif ()
endif ()

if ( EMERAUDE_ENABLE_CPUFEATURES )
	include(SetupCPUFeatures)
endif ()

if ( EMERAUDE_ENABLE_HWLOC )
	include(SetupHWLOC)
endif ()

if ( EMERAUDE_ENABLE_IMGUI )
	include(SetupPostSourcesImGui)
endif ()

if ( EMERAUDE_ENABLE_BULLET )
	include(SetupBullet)
endif()

if ( EMERAUDE_ENABLE_FASTGLTF )
	include(SetupFastGLTF)
endif()



###### Testing ######

if ( EMERAUDE_ENABLE_TESTS )
	file(GLOB_RECURSE TEST_SOURCE_FILES ${CMAKE_CURRENT_SOURCE_DIR}/src/Testing/*.cpp)

	add_executable(EmeraudeTest ${TEST_SOURCE_FILES})

	set_target_properties(EmeraudeTest PROPERTIES
		CXX_STANDARD 20
		CXX_STANDARD_REQUIRED On
		CXX_EXTENSIONS On
	)

	target_include_directories(EmeraudeTest PRIVATE ${EMERAUDE_SOURCE_DIR})
	target_link_libraries(EmeraudeTest PRIVATE ${PROJECT_NAME})

	# Enabling google test as sub project
	set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)

	add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/dependencies/googletest gtest)

	target_include_directories(EmeraudeTest PRIVATE ${CMAKE_SOURCE_DIR}/dependencies/googletest)
	target_link_libraries(EmeraudeTest PRIVATE gtest)

	enable_testing()

	add_test(NAME EmeraudeTest COMMAND $<TARGET_FILE:EmeraudeTest>)
endif ()



###### Write the project C++ config file ######

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/src/emeraude_config.hpp.in ${CMAKE_BINARY_DIR}/include/emeraude_config.hpp)

if ( EMERAUDE_ENABLE_TSAN )
	message("Use env variable : TSAN_OPTIONS=\"suppressions=tsan_suppressions.txt\"")

	file(
		COPY ${CMAKE_CURRENT_SOURCE_DIR}/resources/tsan_suppressions.txt
		DESTINATION ${EMERAUDE_COMPILATION_DIR}
	)
endif ()

if ( NOT "${EMERAUDE_COMPILATION_DIR}" STREQUAL "" )
	# Copy 'gamecontrollerdb.txt' in the compilation directory.
	if ( APPLE )
		file(
			COPY ${CMAKE_CURRENT_SOURCE_DIR}/dependencies/SDL_GameControllerDB/gamecontrollerdb.txt
			DESTINATION ${EMERAUDE_COMPILATION_DIR}/Contents/Resources/data
		)
	else ()
		file(
			COPY ${CMAKE_CURRENT_SOURCE_DIR}/dependencies/SDL_GameControllerDB/gamecontrollerdb.txt
			DESTINATION ${EMERAUDE_COMPILATION_DIR}/data
		)
	endif ()

	# Copy debug file
	if ( CMAKE_BUILD_TYPE MATCHES Debug )
		file(
			COPY ${CMAKE_CURRENT_SOURCE_DIR}/resources/vk_layer_settings.txt
			DESTINATION ${EMERAUDE_COMPILATION_DIR}
		)
	endif ()

	# Copy test files
	if ( EMERAUDE_ENABLE_TESTS )
		file(
			COPY ${CMAKE_CURRENT_SOURCE_DIR}/resources/test-assets
			DESTINATION ${EMERAUDE_COMPILATION_DIR}
		)
	endif ()
else ()
	message(STATUS "EMERAUDE_COMPILATION_DIR is not set, the compilation directory won't have all files for development ! This is a sub-project warning !")
endif ()
