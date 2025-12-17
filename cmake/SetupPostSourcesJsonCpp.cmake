if ( NOT TARGET_BINARY_FOR_SETUP )
    message(FATAL_ERROR "TARGET_BINARY_FOR_SETUP is not SET !")
endif ()

message("Enabling JsonCpp library from local source ...")

target_include_directories(${TARGET_BINARY_FOR_SETUP} PUBLIC ${JSONCPP_INCLUDE_DIRS})
target_compile_definitions(${TARGET_BINARY_FOR_SETUP} PUBLIC JSON_USE_EXCEPTION=Off)

# Disable specific warnings for jsoncpp sources on Clang
if (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    set_source_files_properties(${JSONCPP_SOURCE_FILES} PROPERTIES COMPILE_FLAGS
        "-Wno-nan-infinity-disabled -Wno-unneeded-internal-declaration")
endif ()
