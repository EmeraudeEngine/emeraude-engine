if ( NOT TARGET_BINARY_FOR_SETUP )
    message(FATAL_ERROR "TARGET_BINARY_FOR_SETUP is not SET !")
endif ()

message("Enabling ImGUI library (Local binary) ...")
message(" - Headers : ${IMGUI_INCLUDE_DIRS}")

target_include_directories(${TARGET_BINARY_FOR_SETUP} SYSTEM PUBLIC ${IMGUI_INCLUDE_DIRS})

# Dear ImGui (third-party): its own object library, the cascade's code generation without its warning set.
add_library(EmeraudeImGui OBJECT ${IMGUI_SOURCE_FILES})

set_target_properties(EmeraudeImGui PROPERTIES
	CXX_STANDARD ${EMERAUDE_CXX_VERSION}
	CXX_STANDARD_REQUIRED On
	CXX_EXTENSIONS Off
	POSITION_INDEPENDENT_CODE On
)

target_include_directories(EmeraudeImGui SYSTEM PRIVATE ${IMGUI_INCLUDE_DIRS} ${CMAKE_CURRENT_SOURCE_DIR}/dependencies/glfw/include "${EMERAUDE_VULKAN_SDK_DIR}/include" ${CMAKE_CURRENT_SOURCE_DIR}/dependencies/volk)
target_compile_definitions(EmeraudeImGui PRIVATE ${EMERAUDE_COMPILE_DEFINITIONS} GLFW_INCLUDE_VULKAN GLFW_INCLUDE_NONE VK_NO_PROTOTYPES IMGUI_IMPL_VULKAN_USE_VOLK)
target_compile_definitions(${TARGET_BINARY_FOR_SETUP} PRIVATE IMGUI_IMPL_VULKAN_USE_VOLK)
target_compile_options(EmeraudeImGui PRIVATE ${EMERAUDE_THIRD_PARTY_COMPILE_OPTIONS})

if ( APPLE )
	target_compile_options(EmeraudeImGui PRIVATE "-idirafter/usr/local/include")
endif ()

target_sources(${TARGET_BINARY_FOR_SETUP} PRIVATE $<TARGET_OBJECTS:EmeraudeImGui>)

set(IMGUI_ENABLED On)
