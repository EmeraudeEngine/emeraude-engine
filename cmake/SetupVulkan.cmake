if ( NOT TARGET_BINARY_FOR_SETUP )
	message(FATAL_ERROR "TARGET_BINARY_FOR_SETUP is not SET !")
endif ()

set(VULKAN_SDK_VERSION "1.4.309.0")

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

	# On some project, the clang compiler refuse to look at '/usr/local/include'
	target_compile_options(${TARGET_BINARY_FOR_SETUP} PUBLIC "-I/usr/local/include")
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
# the initial find_package and the transitive ones at LOCAL_LIB_DIR.
list(PREPEND CMAKE_PREFIX_PATH ${LOCAL_LIB_DIR})

find_package(glslang CONFIG REQUIRED PATHS ${LOCAL_LIB_DIR} NO_DEFAULT_PATH)

# Headers are already included via ${LOCAL_LIB_DIR}/include in the main CMakeLists.txt.
target_link_libraries(${TARGET_BINARY_FOR_SETUP} PRIVATE glslang::SPIRV glslang::glslang)
