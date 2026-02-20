if ( NOT TARGET_BINARY_FOR_SETUP )
	message(FATAL_ERROR "TARGET_BINARY_FOR_SETUP is not SET !")
endif ()

message("[EmeraudeEngine] Enabling RenderDoc in-application API (submodule: dependencies/renderdoc) ...")

set(RENDERDOC_INCLUDE_DIR "${CMAKE_CURRENT_LIST_DIR}/../dependencies/renderdoc/renderdoc/api/app")

if ( NOT EXISTS "${RENDERDOC_INCLUDE_DIR}/renderdoc_app.h" )
	message(FATAL_ERROR "[EmeraudeEngine] RenderDoc submodule not found! Run: git submodule update --init dependencies/renderdoc")
endif ()

target_include_directories(${TARGET_BINARY_FOR_SETUP} PUBLIC ${RENDERDOC_INCLUDE_DIR})
target_compile_definitions(${TARGET_BINARY_FOR_SETUP} PUBLIC EMERAUDE_ENABLE_RENDERDOC)

if ( UNIX )
	target_link_libraries(${TARGET_BINARY_FOR_SETUP} PUBLIC ${CMAKE_DL_LIBS})
endif ()

set(RENDERDOC_ENABLED On)
