#######################################################################
## CMake configuration
cmake_minimum_required(VERSION 3.25.1)
cmake_policy(SET CMP0042 NEW) # MACOSX_RPATH is enabled by default.
cmake_policy(SET CMP0091 NEW) # MSVC runtime library flags are selected by an abstraction.
#cmake_policy(SET CMP0177 NEW) # Normalize all install path 3.31

# Adds custom CMake scripts/modules
list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

message("[EmeraudeEngine] Using CMake ${CMAKE_VERSION}")

#######################################################################
## Configure the parallel job count.
cmake_host_system_information(RESULT DETECTED_JOB_COUNT QUERY NUMBER_OF_PHYSICAL_CORES)

set(JOB_COUNT ${DETECTED_JOB_COUNT} CACHE STRING "The number of job used for compilation. Default auto.")
set(CMAKE_BUILD_PARALLEL_LEVEL ${JOB_COUNT})

message("[EmeraudeEngine] Job processes used : ${CMAKE_BUILD_PARALLEL_LEVEL}")

#######################################################################
## Declare project and options.
option(EMERAUDE_BUILD_SERVICES_ONLY "Build only the Emeraude-Engine services (Default Off)" Off)

if ( EMERAUDE_BUILD_SERVICES_ONLY )
	set(BINARY_NAME EmeraudeServices)
else ()
	set(BINARY_NAME Emeraude)
endif ()

project(
	${BINARY_NAME}
	VERSION 0.9.33
	DESCRIPTION "Emeraude Engine is an open-source (LGPLv3) 3D Graphics engine using Vulkan and written in C++20."
	HOMEPAGE_URL "https://github.com/EmeraudeEngine/emeraude-engine"
	LANGUAGES C CXX
)

set_property(GLOBAL PROPERTY USE_FOLDERS On)

set(CMAKE_VERBOSE_MAKEFILE On)
set(CMAKE_EXPORT_COMPILE_COMMANDS On)

#######################################################################
## Build type
set(CMAKE_CONFIGURATION_TYPES Debug Release RelWithDebInfo MinSizeRel)

if ( MSVC )
	if ( MSVC_BUILD_TYPE )
		set(CMAKE_BUILD_TYPE ${MSVC_BUILD_TYPE})
	elseif ( NOT CMAKE_BUILD_TYPE )
		message(FATAL_ERROR "[EmeraudeEngine] Set -DMSVC_BUILD_TYPE=Debug|Release|RelWithDebInfo|MinSizeRel on Windows.")
	endif ()
elseif ( NOT CMAKE_BUILD_TYPE )
	set(CMAKE_BUILD_TYPE Release)
endif ()

message("[EmeraudeEngine] Build type : ${CMAKE_BUILD_TYPE}")

# Configure output directory by compilation type.
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/$<CONFIG>")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/$<CONFIG>")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/$<CONFIG>")

# Emeraude libraries project variables and options.
set(EMERAUDE_COMPILATION_DIR "" CACHE STRING "Declare where the project is compiled. For macOS this should point the 'package.app' in the compilation folder.")
option(EMERAUDE_USE_STATIC_RUNTIME "Build Emeraude-Engine with the static MSVC runtime (Windows only, Default Off)." Off)
option(EMERAUDE_ENABLE_ASAN "Enable 'AddressSanitizer' library. Adds a dev tool to detects memory errors [SYSTEM] (Linux only, Default Off)." Off)
option(EMERAUDE_ENABLE_MSAN "Enable 'MemorySanitizer' library. Adds a dev tool to detects memory corruption [SYSTEM] (Linux only, Default Off)." Off)
option(EMERAUDE_ENABLE_LSAN "Enable 'LeakSanitizer' library. Adds a dev tool to detects memory leak [SYSTEM] (Linux only, Default Off)." Off)
option(EMERAUDE_ENABLE_TSAN "Enable 'ThreadSanitizer' library. Adds a dev tool to detects data race [SYSTEM] (Linux only, Default Off)." Off)
option(EMERAUDE_ENABLE_UBSAN "Enable 'UndefinedBehaviorSanitizer' library. Adds a dev tool to detects undefined behavior [SYSTEM] (Linux only, Default Off)." Off)
option(EMERAUDE_ENABLE_IMGUI "Enable 'libimgui' library. Adds a GUI to the engine [LOCAL] (Default Off)." Off)
# Debug preprocessor macros control (Ignored in Release).
option(EMERAUDE_DEBUG_KEYBOARD_INPUT "Enable printing of events to the console from keyboard inputs (Default Off)." Off)
option(EMERAUDE_DEBUG_POINTER_INPUT "Enable printing of events to the console from pointer inputs (Default Off)." Off)
option(EMERAUDE_DEBUG_POINTER_HEAVY_INPUT "Enable printing of events to the console from pointer heavy (move/wheel) inputs (Default Off)." Off)
option(EMERAUDE_DEBUG_WINDOW_EVENTS "Enable printing of events to the console from window events (Default Off)." Off)
option(EMERAUDE_DEBUG_VULKAN_TRACKING "Enable printing of Vulkan objects lifespan (Default Off)." Off)
option(EMERAUDE_ENABLE_RENDERDOC "Enable RenderDoc in-application API for GPU frame capture [FETCHED] (Default Off)." Off)

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

	else ()
		message("[EmeraudeEngine] Using dynamic MSVC Runtime library!")

		if ( NOT CMAKE_MSVC_RUNTIME_LIBRARY )
			# Set /MD or /MDd compiler flag (Default behavior).
			set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL")
		else ()
			message("[EmeraudeEngine] CMAKE_MSVC_RUNTIME_LIBRARY variable already setup from top project. Skipping ...")
		endif ()

	endif ()

	# Remove default MSVC flags added by CMake to avoid D9025 warnings
	string(REPLACE "/EHsc" "" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
	string(REPLACE "/Ob2" "" CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE}")
endif ()

#######################################################################
## Foundation library (emeraude-base)
# Provides Base/*, platform/config headers, and the external dependencies. Cloned if
# absent, then add_subdirectory'd. Pulled in BEFORE the engine target so its targets
# (emeraude::base*) and exported variables (EMERAUDE_EXT_LIBS_* …) are available.
include(InstallEmeraudeBase)

# emeraude-base owns the Setup*.cmake for every ext-deps-generator library (incl. ones only
# the engine uses: ufbx, fastgltf, taglib, bc7enc, cpu_features, hwloc, libvpx). Add its
# cmake/ dir so the engine can include(Setup<Lib>) from there.
list(APPEND CMAKE_MODULE_PATH ${EMERAUDE_BASE_CMAKE_DIR})

# Print out project paths
message("[EmeraudeEngine] Project directory: ${CMAKE_SOURCE_DIR}, current : ${CMAKE_CURRENT_SOURCE_DIR}")
message("[EmeraudeEngine] Binary directory: ${CMAKE_BINARY_DIR}, current : ${CMAKE_CURRENT_BINARY_DIR}")
message("[EmeraudeEngine] Local external dependencies: ${EMERAUDE_EXT_LIBS_PATH}")

#######################################################################
## Pre sources assembly operations.
if ( EMERAUDE_ENABLE_IMGUI )
	include(SetupPreSourcesImGui)
endif ()

#######################################################################
## Project header and source files.
include(PrepareEngineSourceFiles)

# ImGui library source files
if ( EMERAUDE_ENABLE_IMGUI )
	list(APPEND EMERAUDE_SOURCE_FILES ${IMGUI_SOURCE_FILES})
endif ()

#######################################################################
## Declare the library target and configure it per platform.
# NOTE: As the library is published through LGPLv3 licence, the binary must be a shared library.
add_library(${PROJECT_NAME} SHARED ${EMERAUDE_SOURCE_FILES})

# TODO: Change this by creating a real API with "emeraude_export.hpp", set all classes/functions like : class MaClasse; class EMERAUDE_API MaClasse;
#target_compile_definitions(Emeraude PRIVATE EMERAUDE_BUILD_DLL)

target_sources(${PROJECT_NAME} PUBLIC FILE_SET HEADERS BASE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/src FILES ${EMERAUDE_HEADER_FILES})

target_link_directories(${PROJECT_NAME} PUBLIC ${EMERAUDE_EXT_LIBS_LIB_DIR})

# Link the emeraude-base foundation library. PUBLIC so its headers and external
# dependencies propagate to the engine sources and downstream consumers (projet-alpha).
target_link_libraries(${PROJECT_NAME} PUBLIC emeraude::base)

# Platform-specific target properties.
if ( MSVC )
	message("[EmeraudeEngine] Configuring C++ compiler (>= MSVC 2022) for Windows ...")

	# Compile definitions/options come from emeraude-base (EMERAUDE_COMPILE_*).

	set_target_properties(${PROJECT_NAME} PROPERTIES
		C_STANDARD ${EMERAUDE_C_VERSION}
		C_STANDARD_REQUIRED On
		C_EXTENSIONS Off
		CXX_STANDARD ${EMERAUDE_CXX_VERSION}
		CXX_STANDARD_REQUIRED On
		CXX_EXTENSIONS Off
		VERSION ${PROJECT_VERSION}
		WINDOWS_EXPORT_ALL_SYMBOLS ON # TODO: Change this by creating a real API with "emeraude_export.hpp"
	)

	# Link Secur32 for GetUserNameExW (Extended Security API).
	target_link_libraries(${PROJECT_NAME} PRIVATE Secur32)
elseif ( APPLE )
	message("[EmeraudeEngine] Configuring C++ compiler (>= AppleClang 17.0) for macOS ...")

	# Compile definitions/options come from emeraude-base (EMERAUDE_COMPILE_*).
	set_target_properties(${PROJECT_NAME} PROPERTIES
		C_STANDARD ${EMERAUDE_C_VERSION}
		C_STANDARD_REQUIRED On
		C_EXTENSIONS Off
		CXX_STANDARD ${EMERAUDE_CXX_VERSION}
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

	# Link UniformTypeIdentifiers framework for UTType (macOS 11.0+)
	target_link_libraries(${PROJECT_NAME} PRIVATE "-framework UniformTypeIdentifiers")

	# Link CoreWLAN framework for WiFi scanner.
	target_link_libraries(${PROJECT_NAME} PRIVATE "-framework CoreWLAN")

	# Link DiskArbitration framework for disk info (removable detection).
	target_link_libraries(${PROJECT_NAME} PRIVATE "-framework DiskArbitration")
elseif ( UNIX )
	message("[EmeraudeEngine] Configuring C++ compiler (>= G++13.3) for Linux ...")

	# Compile definitions/options come from emeraude-base (EMERAUDE_COMPILE_*).
	set_target_properties(${PROJECT_NAME} PROPERTIES
		C_STANDARD ${EMERAUDE_C_VERSION}
		C_STANDARD_REQUIRED On
		C_EXTENSIONS Off
		CXX_STANDARD ${EMERAUDE_CXX_VERSION}
		CXX_STANDARD_REQUIRED On
		CXX_EXTENSIONS Off
		VERSION ${PROJECT_VERSION}
		SOVERSION ${PROJECT_VERSION_MAJOR}
	)

	# Linux-only sanitizer options.
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
else ()
	message(FATAL_ERROR "[EmeraudeEngine] Unsupported platform/compiler! NOTE: If you are on Windows be sure to use 'Visual Studio 2022' (>=).")
endif ()

target_compile_definitions(${PROJECT_NAME} PUBLIC ${EMERAUDE_COMPILE_DEFINITIONS})

message("[EmeraudeEngine] '${PROJECT_NAME}' will be compiled with definitions: ${EMERAUDE_COMPILE_DEFINITIONS}")

target_compile_options(${PROJECT_NAME} PUBLIC ${EMERAUDE_COMPILE_OPTIONS})

message("[EmeraudeEngine] '${PROJECT_NAME}' will be compiled with flags: ${EMERAUDE_COMPILE_OPTIONS}")

target_include_directories(${PROJECT_NAME} SYSTEM PUBLIC
    ${CMAKE_BINARY_DIR}/include
    ${EMERAUDE_EXT_LIBS_INCLUDE_DIR}
)

#######################################################################
## Dependencies
# Common cmake switch to turn to OFF for ever below dependencies.
set(BUILD_SHARED_LIBS Off CACHE BOOL "" FORCE)

# Set to what binary the dependencies will be linked.
set(TARGET_BINARY_FOR_SETUP ${PROJECT_NAME})

# Magic Enum (engine-only)
target_include_directories(${PROJECT_NAME} SYSTEM PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/dependencies/magic_enum/include)

# TagLib (engine-only audio metadata)
if ( NOT EMERAUDE_BUILD_SERVICES_ONLY )
	include(SetupTagLib)
endif ()

include(SetupCPUFeatures)

include(SetupHWLOC)

if ( NOT EMERAUDE_BUILD_SERVICES_ONLY )
	# Threads + OpenMP are provided by emeraude::base (used by its ThreadPool / VertexFactory) and inherited transitively.

	include(SetupVulkan) # Using add_subdirectory()

	include(SetupGLFW) # Using add_subdirectory()

	include(SetupReproc) # Using add_subdirectory()

	include(SetupFastGLTF)

	include(SetupUFBX) # FBX parsing library

	include(SetupBC7EncRDO) # BC7 texture compression

	include(SetupOpenAL)

	if ( EMERAUDE_ENABLE_IMGUI )
		include(SetupPostSourcesImGui)
	endif ()

	include(SetupVideoDeviceCapture)

	include(SetupLibVPX)

	if ( EMERAUDE_ENABLE_RENDERDOC )
		include(SetupRenderDoc)
	endif ()
else ()
	# NOTE: Work-around for multi-builds
	if ( EMERAUDE_ENABLE_IMGUI )
		set(IMGUI_ENABLED On)
	endif ()
endif ()

unset(TARGET_BINARY_FOR_SETUP)

#######################################################################
## Write the project C++ config file
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
	if ( CMAKE_BUILD_TYPE STREQUAL "Debug" )
		add_custom_target(copy_debug_files ALL
			COMMAND ${CMAKE_COMMAND} -E copy_if_different
			"${CMAKE_CURRENT_SOURCE_DIR}/resources/vk_layer_settings.txt"
			"${EMERAUDE_COMPILATION_DIR}/vk_layer_settings.txt"
			VERBATIM
		)
	endif ()

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
else ()
	message(STATUS "[EmeraudeEngine] EMERAUDE_COMPILATION_DIR is not set, the compilation directory won't have all files for development ! This is a sub-project warning !")
endif ()
