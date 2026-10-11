if ( NOT TARGET_BINARY_FOR_SETUP )
	message(FATAL_ERROR "TARGET_BINARY_FOR_SETUP is not SET !")
endif ()

# The Vulkan SDK comes from the external dependencies, like every other native dependency: the strict minimum of the
# LunarG SDK (headers, link library, validation layer, MoltenVK and the loader on macOS), extracted by ext-deps-generator
# (extract_vulkan_sdk.py), which owns its version. No installed SDK is used, so none is needed.
set(EMERAUDE_VULKAN_SDK_DIR "${EMERAUDE_EXT_LIBS_PATH}/vulkan-sdk")

if ( NOT EXISTS "${EMERAUDE_VULKAN_SDK_DIR}/VERSION" )
	message(FATAL_ERROR "The external dependencies carry no Vulkan SDK ('${EMERAUDE_VULKAN_SDK_DIR}'). Use an archive that has it, or run ext-deps-generator's extract_vulkan_sdk.py for a local output.")
endif ()

file(STRINGS "${EMERAUDE_VULKAN_SDK_DIR}/VERSION" _emeraudeVulkanSDKVersion LIMIT_COUNT 1)

# NOTE: The engine links NO Vulkan library: it opens the vulkan-sdk loader by explicit path at run time
# (Vulkan::Loader, owned by PlatformManager) and fetches every function through volk. CEF keeps its own loader for
# its GPU process. Owner decision 2026-10-11.
target_include_directories(${TARGET_BINARY_FOR_SETUP} SYSTEM PUBLIC "${EMERAUDE_VULKAN_SDK_DIR}/include" "${CMAKE_CURRENT_SOURCE_DIR}/dependencies/volk")
target_compile_definitions(${TARGET_BINARY_FOR_SETUP} PUBLIC VK_NO_PROTOTYPES)

# volk (Arseny Kapoulkine, MIT, https://github.com/zeux/volk, pinned to the SDK's tag) is third-party C code: its own
# object library, the cascade's code generation without its warning set.
add_library(EmeraudeVolk OBJECT ${CMAKE_CURRENT_SOURCE_DIR}/dependencies/volk/volk.c)
set_target_properties(EmeraudeVolk PROPERTIES POSITION_INDEPENDENT_CODE On)
target_include_directories(EmeraudeVolk SYSTEM PRIVATE "${EMERAUDE_VULKAN_SDK_DIR}/include")
target_compile_options(EmeraudeVolk PRIVATE ${EMERAUDE_THIRD_PARTY_COMPILE_OPTIONS})
target_sources(${TARGET_BINARY_FOR_SETUP} PRIVATE $<TARGET_OBJECTS:EmeraudeVolk>)

if ( UNIX )
	target_link_libraries(${TARGET_BINARY_FOR_SETUP} PRIVATE ${CMAKE_DL_LIBS})
endif ()

message("Vulkan SDK ${_emeraudeVulkanSDKVersion} enabled from the external dependencies (headers + volk, loader opened at run time) !")
message(" - Headers : ${EMERAUDE_VULKAN_SDK_DIR}/include")


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
