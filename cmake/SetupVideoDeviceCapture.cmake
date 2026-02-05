if ( NOT TARGET_BINARY_FOR_SETUP )
    message(FATAL_ERROR "TARGET_BINARY_FOR_SETUP is not SET !")
endif ()

message("Enabling Video Device Capture support ...")

if ( APPLE )
    target_link_libraries(${TARGET_BINARY_FOR_SETUP} PRIVATE
        "-framework AVFoundation"
        "-framework CoreMedia"
        "-framework CoreVideo"
    )
endif ()

if ( MSVC )
    target_link_libraries(${TARGET_BINARY_FOR_SETUP} PRIVATE
        Mfplat.lib
        Mfreadwrite.lib
        Mf.lib
        Mfuuid.lib
    )
endif ()

# Linux: V4L2 uses kernel headers only, no external library needed.
