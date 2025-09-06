if ( NOT TARGET_BINARY_FOR_SETUP )
    message(FATAL_ERROR "TARGET_BINARY_FOR_SETUP is not SET !")
endif ()

message("Enabling ImGUI library (Local binary) ...")
message(" - Headers : ${IMGUI_INCLUDE_DIRS}")

target_include_directories(${TARGET_BINARY_FOR_SETUP} PUBLIC ${IMGUI_INCLUDE_DIRS})

set(IMGUI_ENABLED On)
