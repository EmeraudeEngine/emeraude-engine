if ( NOT TARGET_BINARY_FOR_SETUP )
	message(FATAL_ERROR "TARGET_BINARY_FOR_SETUP is not SET !")
endif ()

set(VULKAN_SDK_VERSION "1.4.309.0")

if ( UNIX AND NOT APPLE )
	find_package(Vulkan REQUIRED)

	target_include_directories(${TARGET_BINARY_FOR_SETUP} PUBLIC ${Vulkan_INCLUDE_DIRS})

	target_link_libraries(${TARGET_BINARY_FOR_SETUP} PRIVATE Vulkan::Vulkan)
elseif ( APPLE )
	set(VULKAN_SDK_PATH "$ENV{HOME}/VulkanSDK/${VULKAN_SDK_VERSION}/")

	if ( NOT EXISTS ${VULKAN_SDK_PATH} )
		message(FATAL_ERROR "The Vulkan SDK is not found in '${VULKAN_SDK_PATH}' ! You can download it from: https://sdk.lunarg.com/sdk/download/${VULKAN_SDK_VERSION}/mac/vulkansdk-macos-${VULKAN_SDK_VERSION}.zip")
	endif ()

	find_package(Vulkan REQUIRED)

	target_include_directories(${TARGET_BINARY_FOR_SETUP} PUBLIC ${Vulkan_INCLUDE_DIRS})

	target_link_libraries(${TARGET_BINARY_FOR_SETUP} PRIVATE Vulkan::Vulkan)

	# Fails on M4, because it integrate MoltenVK inside the binary.
	#find_package(Vulkan REQUIRED COMPONENTS MoltenVK)
	#target_include_directories(${TARGET_BINARY_FOR_SETUP} PUBLIC ${Vulkan_INCLUDE_DIRS})
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

	target_include_directories(${TARGET_BINARY_FOR_SETUP} PUBLIC ${Vulkan_INCLUDE_DIRS})

	target_link_libraries(${TARGET_BINARY_FOR_SETUP} PRIVATE Vulkan::Vulkan)
endif ()

message("Vulkan ${Vulkan_VERSION} SDK enabled !")
message(" - Headers : ${Vulkan_INCLUDE_DIRS}")
message(" - Binary : ${Vulkan_LIBRARIES}")


# Vulkan Memory Allocator
message("Configuring Vulkan Memory Allocator library as sub-project ...")

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/dependencies/VulkanMemoryAllocator EXCLUDE_FROM_ALL)

target_include_directories(${TARGET_BINARY_FOR_SETUP} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/dependencies/VulkanMemoryAllocator/include)

target_link_libraries(${TARGET_BINARY_FOR_SETUP} PRIVATE VulkanMemoryAllocator)


# GLSLang
message("Configuring GLSLang library as sub-project ...")

if ( NOT EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/dependencies/glslang/External/spirv-tools )
	message("Launching '${CMAKE_CURRENT_SOURCE_DIR}/dependencies/glslang/update_glslang_sources.py' ...")

	find_package(Python3 REQUIRED COMPONENTS Interpreter)

	execute_process(
		COMMAND ${Python3_EXECUTABLE} update_glslang_sources.py
		WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/dependencies/glslang
		COMMAND_ERROR_IS_FATAL ANY
	)
endif ()

set(GLSLANG_TESTS_DEFAULT Off CACHE BOOL "" FORCE)
set(GLSLANG_ENABLE_INSTALL_DEFAULT Off CACHE BOOL "" FORCE)
set(ENABLE_GLSLANG_BINARIES Off CACHE BOOL "" FORCE)
set(ENABLE_GLSLANG_JS Off CACHE BOOL "" FORCE)
set(ENABLE_HLSL Off CACHE BOOL "" FORCE)
set(ENABLE_RTTI Off CACHE BOOL "" FORCE)
set(ENABLE_EXCEPTIONS Off CACHE BOOL "" FORCE)

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/dependencies/glslang EXCLUDE_FROM_ALL)

target_include_directories(${TARGET_BINARY_FOR_SETUP} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/dependencies/glslang)

target_link_libraries(${TARGET_BINARY_FOR_SETUP} PRIVATE SPIRV glslang)
