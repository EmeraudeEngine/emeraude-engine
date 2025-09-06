if ( NOT TARGET_BINARY_FOR_SETUP )
    message(FATAL_ERROR "TARGET_BINARY_FOR_SETUP is not SET !")
endif ()

message("Enabling JsonCpp library from local source ...")

target_include_directories(${TARGET_BINARY_FOR_SETUP} PUBLIC ${JSONCPP_INCLUDE_DIRS})
target_compile_definitions(${TARGET_BINARY_FOR_SETUP} PUBLIC JSON_USE_EXCEPTION=Off)
