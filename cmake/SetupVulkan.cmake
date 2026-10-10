if ( NOT TARGET_BINARY_FOR_SETUP )
	message(FATAL_ERROR "TARGET_BINARY_FOR_SETUP is not SET !")
endif ()

set(VULKAN_SDK_VERSION "1.4.363.0")

if ( UNIX AND NOT APPLE )
	find_package(Vulkan REQUIRED)

	target_include_directories(${TARGET_BINARY_FOR_SETUP} SYSTEM PUBLIC ${Vulkan_INCLUDE_DIRS})

	target_link_libraries(${TARGET_BINARY_FOR_SETUP} PRIVATE Vulkan::Vulkan)
elseif ( APPLE )
	set(VULKAN_SDK_PATH "$ENV{HOME}/VulkanSDK/${VULKAN_SDK_VERSION}/")

	if ( NOT EXISTS ${VULKAN_SDK_PATH} )
		message(FATAL_ERROR "The Vulkan SDK is not found in '${VULKAN_SDK_PATH}' ! You can download it from: https://sdk.lunarg.com/sdk/download/${VULKAN_SDK_VERSION}/mac/vulkansdk-macos-${VULKAN_SDK_VERSION}.zip")
	endif ()

	find_package(Vulkan REQUIRED)

	target_include_directories(${TARGET_BINARY_FOR_SETUP} SYSTEM PUBLIC ${Vulkan_INCLUDE_DIRS})

	target_link_libraries(${TARGET_BINARY_FOR_SETUP} PRIVATE Vulkan::Vulkan)

	# Fails on M4, because it integrate MoltenVK inside the binary.
	#find_package(Vulkan REQUIRED COMPONENTS MoltenVK)
	#target_include_directories(${TARGET_BINARY_FOR_SETUP} SYSTEM PUBLIC ${Vulkan_INCLUDE_DIRS})
	#target_link_libraries(${TARGET_BINARY_FOR_SETUP} PRIVATE Vulkan::Vulkan Vulkan::MoltenVK "-framework Metal" "-framework AppKit" "-framework QuartzCore" "-framework IOSurface" "-framework Foundation")

	# The Vulkan SDK installs its headers in '/usr/local/include', which some configurations do not search (a
	# sysroot). It is added with -idirafter, NOT -I: a -I directory is searched BEFORE every -isystem one and is judged
	# as our own code. As -I, it made '<glslang/...>' resolve to the SDK's glslang (16.4) instead of the ext-deps one the
	# engine links (16.5): glslang::SpvOptions gained a field in between, so GlslangToSpv() read the engine's options
	# one field off and one byte past the object. It also put the SDK's macros (VK_MAKE_VERSION) and headers under the
	# paranoid warning set. -idirafter is a system directory searched after all the others.
	target_compile_options(${TARGET_BINARY_FOR_SETUP} PUBLIC "-idirafter/usr/local/include")
elseif ( MSVC )
	set(VULKAN_SDK_PATH "C:/VulkanSDK/${VULKAN_SDK_VERSION}/")

	if ( NOT EXISTS ${VULKAN_SDK_PATH} )
		message(FATAL_ERROR "The Vulkan SDK is not found in '${VULKAN_SDK_PATH}' ! You can download it from: https://sdk.lunarg.com/sdk/download/${VULKAN_SDK_VERSION}/windows/VulkanSDK-${VULKAN_SDK_VERSION}-Installer.exe")
	endif ()

	set(ENV{VULKAN_SDK} ${VULKAN_SDK_PATH})

	find_package(Vulkan REQUIRED)

	target_include_directories(${TARGET_BINARY_FOR_SETUP} SYSTEM PUBLIC ${Vulkan_INCLUDE_DIRS})

	target_link_libraries(${TARGET_BINARY_FOR_SETUP} PRIVATE Vulkan::Vulkan)
endif ()

message("Vulkan ${Vulkan_VERSION} SDK enabled !")
message(" - Headers : ${Vulkan_INCLUDE_DIRS}")
message(" - Binary : ${Vulkan_LIBRARIES}")


# Vulkan Memory Allocator
message("Configuring Vulkan Memory Allocator library as sub-project ...")

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/dependencies/VulkanMemoryAllocator EXCLUDE_FROM_ALL)

# Mark VMA include directory as SYSTEM to suppress warnings from header-only library
target_include_directories(${TARGET_BINARY_FOR_SETUP} SYSTEM PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/dependencies/VulkanMemoryAllocator/include)

target_link_libraries(${TARGET_BINARY_FOR_SETUP} PRIVATE VulkanMemoryAllocator)

# GLSLang
message("Enabling GLSLang (with SPIR-V codegen) from local precompiled source ...")

# glslang's exported config calls find_dependency(SPIRV-Tools-opt), which itself
# pulls SPIRV-Tools. find_dependency follows CMAKE_PREFIX_PATH, so we point both
# the initial find_package and the transitive ones at EMERAUDE_EXT_LIBS_PATH.
list(PREPEND CMAKE_PREFIX_PATH ${EMERAUDE_EXT_LIBS_PATH})

find_package(glslang CONFIG REQUIRED PATHS ${EMERAUDE_EXT_LIBS_PATH} NO_DEFAULT_PATH)

# Headers are already included via ${EMERAUDE_EXT_LIBS_PATH}/include in the main CMakeLists.txt.
target_link_libraries(${TARGET_BINARY_FOR_SETUP} PRIVATE glslang::SPIRV glslang::glslang)
