#######################################################################
# CMake configuration
cmake_minimum_required(VERSION 3.25.1)
cmake_policy(SET CMP0042 NEW) # MACOSX_RPATH is enabled by default.
cmake_policy(SET CMP0091 NEW) # MSVC runtime library flags are selected by an abstraction.
#cmake_policy(SET CMP0177 NEW) # Normalize all install path 3.31

# Adds custom CMake scripts/modules
list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

message("[EmeraudeEngine] Using CMake ${CMAKE_VERSION}")

#######################################################################
# Configure the parallel job count.
cmake_host_system_information(RESULT DETECTED_JOB_COUNT QUERY NUMBER_OF_PHYSICAL_CORES)

set(JOB_COUNT ${DETECTED_JOB_COUNT} CACHE STRING "The number of job used for compilation. Default auto.")
set(CMAKE_BUILD_PARALLEL_LEVEL ${JOB_COUNT})

message("[EmeraudeEngine] Job processes used : ${CMAKE_BUILD_PARALLEL_LEVEL}")



#######################################################################
# Declare project and options.
option(EMERAUDE_BUILD_SERVICES_ONLY "Build only the Emeraude-Engine services (Default Off)" Off)

if ( EMERAUDE_BUILD_SERVICES_ONLY )
	set(BINARY_NAME EmeraudeServices)
else ()
	set(BINARY_NAME Emeraude)
endif ()

project(
	${BINARY_NAME}
	VERSION 0.8.2
	DESCRIPTION "Emeraude Engine is an open-source (LGPLv3) 3D Graphics engine using Vulkan and written in C++20."
	HOMEPAGE_URL "https://github.com/EmeraudeEngine/emeraude-engine"
	LANGUAGES C CXX
)

set_property(GLOBAL PROPERTY USE_FOLDERS On)

set(CMAKE_VERBOSE_MAKEFILE On)
set(CMAKE_EXPORT_COMPILE_COMMANDS On)

message("[EmeraudeEngine] Build type : ${CMAKE_BUILD_TYPE}")

# Configure output directory by compilation type.
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/$<CONFIG>")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/$<CONFIG>")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/$<CONFIG>")

# Emeraude libraries project variables and options.
set(EMERAUDE_INTERNET_CHECK_DOMAIN "google.com" CACHE STRING "Web domain to check internet connexion (Default google.com).")
set(EMERAUDE_COMPILATION_DIR "" CACHE STRING "Declare where the project is compiled. For macOS this should point the 'package.app' in the compilation folder.")
option(EMERAUDE_ENABLE_TESTS "Enable the Emeraude-Engine testing suite (Default Off)." Off)
option(EMERAUDE_ENABLE_PCH "Enable 'PreCompiled Headers' feature to speed up the compilation (Default Off)." Off)
option(EMERAUDE_USE_STATIC_RUNTIME "Build Emeraude-Engine with the static MSVC runtime (Windows only, Default Off)." Off)
option(EMERAUDE_USE_SYSTEM_LIBS "Use the system for common libraries instead of the embedded ones (Deprecated, Default Off)." Off)
option(EMERAUDE_USE_SYSTEM_OPENAL "Use the system OpenAL-Soft library (Default Off)." Off)
option(EMERAUDE_ENABLE_ASAN "Enable 'AddressSanitizer' library. Adds a dev tool to detects memory errors [SYSTEM] (Linux only, Default Off)." Off)
option(EMERAUDE_ENABLE_MSAN "Enable 'MemorySanitizer' library. Adds a dev tool to detects memory corruption [SYSTEM] (Linux only, Default Off)." Off)
option(EMERAUDE_ENABLE_LSAN "Enable 'LeakSanitizer' library. Adds a dev tool to detects memory leak [SYSTEM] (Linux only, Default Off)." Off)
option(EMERAUDE_ENABLE_TSAN "Enable 'ThreadSanitizer' library. Adds a dev tool to detects data race [SYSTEM] (Linux only, Default Off)." Off)
option(EMERAUDE_ENABLE_UBSAN "Enable 'UndefinedBehaviorSanitizer' library. Adds a dev tool to detects undefined behavior [SYSTEM] (Linux only, Default Off)." Off)
option(EMERAUDE_ENABLE_OPENMP "Enable 'libomp' library [SYSTEM] (Default On)." On)
option(EMERAUDE_ENABLE_ASIO "Enable 'libasio' library. Adds the networking ability [SYSTEM|LOCAL] (Default On)." On)
option(EMERAUDE_ENABLE_IMGUI "Enable 'libimgui' library. Adds a GUI to the engine [LOCAL] (Default Off)." Off)
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
set(EMERAUDE_DEBUG_THREAD_POOL_N "1" CACHE STRING "When debugging ThreadPool, force the number of threads (Default 1).")


# Force the compilation directory for testing (Only if empty)
if ( EMERAUDE_ENABLE_TESTS AND "${EMERAUDE_COMPILATION_DIR}" STREQUAL "" )
	message(STATUS "[EmeraudeEngine] EMERAUDE_ENABLE_TESTS has been enabled but the EMERAUDE_COMPILATION_DIR is not set. Forcing to : '${CMAKE_BINARY_DIR}/${CMAKE_BUILD_TYPE}'.")

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
	message("[EmeraudeEngine] Linux platform detected !")

	if ( NOT DEFINED APP_ARCH )
		set(APP_ARCH "x86_64")
	endif ()

	message("[EmeraudeEngine] Linux platform targets arch ${APP_ARCH}")

	set(LOCAL_LIB_DIR "${CMAKE_CURRENT_SOURCE_DIR}/dependencies/linux.${APP_ARCH}-${CMAKE_BUILD_TYPE}" CACHE STRING "Final directory for local libraries for Linux.")
elseif ( APPLE )
	message("[EmeraudeEngine] Apple platform detected ! macOS architecture : ${CMAKE_OSX_ARCHITECTURES}, default target SDK : ${CMAKE_OSX_DEPLOYMENT_TARGET}")

	if ( "${CMAKE_OSX_ARCHITECTURES}" STREQUAL "x86_64" )
		set(APP_ARCH "x86_64")
		set(APP_OSX_DEPLOYMENT_TARGET "12.0" CACHE STRING "Minimum SDK for macOS.")
	elseif ( "${CMAKE_OSX_ARCHITECTURES}" STREQUAL "arm64" )
		set(APP_ARCH "arm64")
		set(APP_OSX_DEPLOYMENT_TARGET "12.0" CACHE STRING "Minimum SDK for macOS.")
	else ()
		message(FATAL_ERROR "[EmeraudeEngine] Please specify the architecture for macOS with -DCMAKE_OSX_ARCHITECTURES=x86_64 or -DCMAKE_OSX_ARCHITECTURES=arm64 !")
	endif ()

	# NOTE: TO be sure
	set(CMAKE_OSX_ARCHITECTURES ${APP_ARCH})
	set(CMAKE_OSX_DEPLOYMENT_TARGET ${APP_OSX_DEPLOYMENT_TARGET})

	message("[EmeraudeEngine] Apple platform targets arch ${APP_ARCH} with SDK ${APP_OSX_DEPLOYMENT_TARGET}")

	set(LOCAL_LIB_DIR "${CMAKE_CURRENT_SOURCE_DIR}/dependencies/mac.${APP_ARCH}-${CMAKE_BUILD_TYPE}" CACHE STRING "Final directory for local libraries for macOS.")
elseif ( MSVC )
	message("[EmeraudeEngine] Windows platform detected !")

	if ( NOT DEFINED APP_ARCH )
		set(APP_ARCH "x86_64")
	endif ()

	message("[EmeraudeEngine] Windows platform targets arch ${APP_ARCH}")

	if ( EMERAUDE_USE_STATIC_RUNTIME )
		message("[EmeraudeEngine] Using static MSVC Runtime library!")

		if ( NOT CMAKE_MSVC_RUNTIME_LIBRARY )
			# Set /MT or /MTd compiler flag.
			set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
		else ()
			message("[EmeraudeEngine] CMAKE_MSVC_RUNTIME_LIBRARY variable already setup from top project. Skipping ...")
		endif ()

		set(LOCAL_LIB_DIR "${CMAKE_CURRENT_SOURCE_DIR}/dependencies/windows.${APP_ARCH}-${CMAKE_BUILD_TYPE}-MT" CACHE STRING "Final directory for local libraries for Windows.")
	else ()
		message("[EmeraudeEngine] Using dynamic MSVC Runtime library!")

		if ( NOT CMAKE_MSVC_RUNTIME_LIBRARY )
			# Set /MD or /MDd compiler flag (Default behavior).
			set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL")
		else ()
			message("[EmeraudeEngine] CMAKE_MSVC_RUNTIME_LIBRARY variable already setup from top project. Skipping ...")
		endif ()

		set(LOCAL_LIB_DIR "${CMAKE_CURRENT_SOURCE_DIR}/dependencies/windows.${APP_ARCH}-${CMAKE_BUILD_TYPE}-MD" CACHE STRING "Final directory for local libraries for Windows.")
	endif ()
endif ()

# Download the binary precompiled package for the current configuration.
#include(InstallExternalDependencies)

# Print out project paths
message("[EmeraudeEngine] Project directory: ${CMAKE_SOURCE_DIR}, current : ${CMAKE_CURRENT_SOURCE_DIR}")
message("[EmeraudeEngine] Binary directory: ${CMAKE_BINARY_DIR}, current : ${CMAKE_CURRENT_BINARY_DIR}")
message("[EmeraudeEngine] Local external dependencies: ${LOCAL_LIB_DIR}")



#######################################################################
# Pre sources assembly operations.
if ( EMERAUDE_ENABLE_IMGUI )
	include(SetupPreSourcesImGui)
endif ()

include(SetupPreSourcesJsonCpp)


#######################################################################
# Project header and source files.
include(PrepareEngineSourceFiles)

# ImGui library source files
if ( EMERAUDE_ENABLE_IMGUI )
	list(APPEND EMERAUDE_SOURCE_FILES ${IMGUI_SOURCE_FILES})
endif ()

# JsonCpp library source files
list(APPEND EMERAUDE_SOURCE_FILES ${JSONCPP_SOURCE_FILES})



#######################################################################
# Declare the library target and configure it per platform.

# NOTE: As the library is published through LGPLv3 licence, this must be a shared library.
add_library(${PROJECT_NAME} SHARED ${EMERAUDE_SOURCE_FILES})

# TODO: Change this by creating a real API with "emeraude_export.hpp", set all classes/functions like : class MaClasse; class EMERAUDE_API MaClasse;
#target_compile_definitions(Emeraude PRIVATE EMERAUDE_BUILD_DLL)

target_sources(${PROJECT_NAME} PUBLIC FILE_SET HEADERS BASE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/src FILES ${EMERAUDE_HEADER_FILES})

# Precompiled headers
if ( EMERAUDE_ENABLE_PCH )
	target_precompile_headers(${PROJECT_NAME} PUBLIC ${EMERAUDE_HEADER_FILES})
endif ()

target_link_directories(${PROJECT_NAME} PUBLIC ${LOCAL_LIB_DIR}/lib)

# Set DEBUG or NDEBUG macro
target_compile_definitions(${PROJECT_NAME} PRIVATE
	$<$<CONFIG:Debug>:DEBUG>
	$<$<CONFIG:Release>:NDEBUG>
)

if ( MSVC )
	message("[EmeraudeEngine] Configuring C++ compiler (>= MSVC 2022) for Windows ...")

	set_target_properties(${PROJECT_NAME} PROPERTIES
		C_STANDARD 17 # Next standard will be C23
		C_STANDARD_REQUIRED On
		C_EXTENSIONS Off
		CXX_STANDARD 20 # Next standard will be C++23, then C++26.
		CXX_STANDARD_REQUIRED On
		CXX_EXTENSIONS Off
		VERSION ${PROJECT_VERSION}
		WINDOWS_EXPORT_ALL_SYMBOLS ON # TODO: Change this by creating a real API with "emeraude_export.hpp"
	)

	# Windows 7	0x0601
	# Windows 8	0x0602
	# Windows 8.1 0x0603
	# Windows 10/11 0x0A00
	target_compile_definitions(${PROJECT_NAME} PRIVATE _WIN32_WINNT=0x0A00)

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
		$<$<CONFIG:Debug>:
			/RTC1         # Disable optimizations
			/Od           # Enable basic run-time checks
			#/WX           # Treat warnings as errors
			/Zi
		>
		$<$<CONFIG:Relase>:
			/O2           # Optimize for maximum speed
			/Ob2          # Inline any suitable function
			/GF           # Enable string pooling
		>
	)
elseif ( UNIX )
	if ( APPLE )
		message("[EmeraudeEngine] Configuring C++ compiler (>= AppleClang 17.0) for macOS ...")

		set_target_properties(${PROJECT_NAME} PROPERTIES
			C_STANDARD 17 # Next standard will be C23
			C_STANDARD_REQUIRED On
			C_EXTENSIONS Off
			CXX_STANDARD 20 # Next standard will be C++23, then C++26.
			CXX_STANDARD_REQUIRED On
			CXX_EXTENSIONS Off
			VERSION ${PROJECT_VERSION}
			SOVERSION ${PROJECT_VERSION_MAJOR}
			FRAMEWORK TRUE
			FRAMEWORK_VERSION ${PROJECT_VERSION}
			MACOSX_FRAMEWORK_IDENTIFIER org.ln-isle.emeraude
			INSTALL_NAME_DIR "@rpath"
			MACOSX_RPATH TRUE
		)

		# Debug include path
		#target_compile_options(${PROJECT_NAME} PRIVATE -v)

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
		message("[EmeraudeEngine] Configuring C++ compiler (>= G++13.3) for Linux ...")

		set_target_properties(${PROJECT_NAME} PROPERTIES
			C_STANDARD 17 # Next standard will be C23
			C_STANDARD_REQUIRED On
			C_EXTENSIONS On
			CXX_STANDARD 20 # Next standard will be C++23, then C++26.
			CXX_STANDARD_REQUIRED On
			CXX_EXTENSIONS On
			VERSION ${PROJECT_VERSION}
			SOVERSION ${PROJECT_VERSION_MAJOR}
		)

		target_compile_options(${PROJECT_NAME} PRIVATE
			-Wno-unknown-pragmas
			-Wno-unused
			-fno-exceptions
			#-fno-rtti
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

		if ( EMERAUDE_ENABLE_ASAN )
			message("[EmeraudeEngine] Enabling AddressSanitizer tools ...")

			target_compile_options(${PROJECT_NAME} PUBLIC -fsanitize=address)
			target_link_options(${PROJECT_NAME} PUBLIC -fsanitize=address)
		endif ()

		if ( EMERAUDE_ENABLE_MSAN )
			message("[EmeraudeEngine] Enabling MemorySanitizer tools ...")

			target_compile_options(${PROJECT_NAME} PUBLIC -fsanitize=memory)
			target_link_options(${PROJECT_NAME} PUBLIC -fsanitize=memory)
		endif ()

		if ( EMERAUDE_ENABLE_LSAN )
			message("[EmeraudeEngine] Enabling LeakSanitizer tools ...")

			target_compile_options(${PROJECT_NAME} PUBLIC -fsanitize=leak)
			target_link_options(${PROJECT_NAME} PUBLIC -fsanitize=leak)
		endif ()

		if ( EMERAUDE_ENABLE_TSAN )
			message("[EmeraudeEngine] Enabling ThreadSanitizer tools ...")

			target_compile_options(${PROJECT_NAME} PUBLIC -fsanitize=thread)
			target_link_options(${PROJECT_NAME} PUBLIC -fsanitize=thread)
		endif ()

		if ( EMERAUDE_ENABLE_UBSAN )
			message("[EmeraudeEngine] Enabling UndefinedBehaviorSanitizer tools ...")

			target_compile_options(${PROJECT_NAME} PUBLIC -fsanitize=undefined -fsanitize=integer -fsanitize=nullability)
			target_link_options(${PROJECT_NAME} PUBLIC -fsanitize=undefined -fsanitize=integer -fsanitize=nullability)
		endif ()
	endif ()
else ()
	message(FATAL_ERROR "[EmeraudeEngine] Unsupported platform/compiler! NOTE: If you are on Windows be sure to use 'Visual Studio 2022' (>=).")
endif ()

target_include_directories(${PROJECT_NAME} PUBLIC
    ${CMAKE_BINARY_DIR}/include
    ${LOCAL_LIB_DIR}/include
)

# "portable-file-dialogs" library for LINUX
if ( UNIX AND NOT APPLE )
	target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/dependencies/portable-file-dialogs)
endif ()



###### Dependencies ######

# Common cmake switch to turn to OFF for ever below dependencies.
set(BUILD_SHARED_LIBS Off CACHE BOOL "" FORCE)

# Set to what binary the dependencies will be linked.
set(TARGET_BINARY_FOR_SETUP ${PROJECT_NAME})

# Base dependencies
include(SetupPostSourcesJsonCpp)

# Magic Enum
target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/dependencies/magic_enum/include)

# Request : brotli, bzip2, harbuzz, png, zlib
include(SetupFreetype)

# Request : zlib bzip2 xz zstd
include(SetupZIP)

# Request : zlib
include(SetupPNG)

# Request : zlib
if ( NOT EMERAUDE_BUILD_SERVICES_ONLY )
	include(SetupTagLib)
endif ()

if ( NOT EMERAUDE_USE_SYSTEM_LIBS )
	include(SetupBrotli)

	include(SetupHarfbuzz)
endif ()

include(SetupJPEG)

include(SetupZLIB)

include(SetupBZIP2)

include(SetupLZMA)

include(SetupZSTD)

if ( EMERAUDE_ENABLE_ASIO )
	if ( EMERAUDE_ENABLE_TSAN )
		message(WARNING "[EmeraudeEngine] Disabling ASIO when ThreadSanitizer is requested !")
	else ()
		include(SetupASIO)
	endif ()
endif ()

include(SetupSndFile)

include(SetupSampleRate)

include(SetupCPUFeatures)

include(SetupHWLOC)

if ( NOT EMERAUDE_BUILD_SERVICES_ONLY )
	include(SetupThreads)

	include(SetupVulkan) # Using add_subdirectory()

	include(SetupGLFW) # Using add_subdirectory()

	include(SetupReproc) # Using add_subdirectory()

	include(SetupFastGLTF) # Using add_subdirectory()

	if ( EMERAUDE_ENABLE_OPENMP AND NOT APPLE )
		include(SetupOpenMP)
	endif ()

	include(SetupOpenAL)

	if ( EMERAUDE_ENABLE_IMGUI )
		include(SetupPostSourcesImGui)
	endif ()
else ()
	# NOTE: Work-around for multi-builds
	if ( EMERAUDE_ENABLE_IMGUI )
		set(IMGUI_ENABLED On)
	endif ()
endif ()

unset(TARGET_BINARY_FOR_SETUP)


###### Testing ######

if ( EMERAUDE_ENABLE_TESTS )
	message("[EmeraudeEngine] Enabling GoogleTest")

	# Fetch GoogleTest
	include(FetchContent)
	include(GoogleTest)

	FetchContent_Declare(googletest GIT_REPOSITORY https://github.com/google/googletest.git GIT_TAG v1.17.0)

	if ( NOT EMERAUDE_USE_STATIC_RUNTIME )
		set(gtest_force_shared_crt On CACHE BOOL "" FORCE)
	endif ()

	FetchContent_MakeAvailable(googletest)

	# Create the executable
	file(GLOB_RECURSE TEST_SOURCE_FILES ${CMAKE_CURRENT_SOURCE_DIR}/src/Testing/*.cpp)

	add_executable(EmeraudeUnitTests ${TEST_SOURCE_FILES})

	# Configure the compilation
	set_target_properties(EmeraudeUnitTests PROPERTIES CXX_STANDARD 20 CXX_STANDARD_REQUIRED On)
	target_include_directories(EmeraudeUnitTests PRIVATE ${EMERAUDE_SOURCE_DIR})
	target_link_libraries(EmeraudeUnitTests PRIVATE ${PROJECT_NAME} gtest_main)

	# Enable the testing
	enable_testing()

	gtest_discover_tests(EmeraudeUnitTests)
endif ()



###### Write the project C++ config file ######

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/src/emeraude_config.hpp.in ${CMAKE_BINARY_DIR}/include/emeraude_config.hpp)

if ( EMERAUDE_ENABLE_TSAN )
	message("[EmeraudeEngine] Use env variable : TSAN_OPTIONS=\"suppressions=tsan_suppressions.txt\"")

	file(
		COPY ${CMAKE_CURRENT_SOURCE_DIR}/resources/tsan_suppressions.txt
		DESTINATION ${EMERAUDE_COMPILATION_DIR}
	)
endif ()

if ( NOT "${EMERAUDE_COMPILATION_DIR}" STREQUAL "" )
	# Copy debug file
	add_custom_target(copy_debug_files ALL
		COMMAND $<$<CONFIG:Debug>:${CMAKE_COMMAND} -E copy_if_different
		"${CMAKE_CURRENT_SOURCE_DIR}/resources/vk_layer_settings.txt"
		"${EMERAUDE_COMPILATION_DIR}/vk_layer_settings.txt" >
		VERBATIM
	)

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

	# Copy test files
	if ( EMERAUDE_ENABLE_TESTS )
		file(
			COPY ${CMAKE_CURRENT_SOURCE_DIR}/resources/test-assets
			DESTINATION ${EMERAUDE_COMPILATION_DIR}
		)
	endif ()
else ()
	message(STATUS "[EmeraudeEngine] EMERAUDE_COMPILATION_DIR is not set, the compilation directory won't have all files for development ! This is a sub-project warning !")
endif ()
