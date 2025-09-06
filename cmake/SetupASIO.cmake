if ( NOT TARGET_BINARY_FOR_SETUP )
	message(FATAL_ERROR "TARGET_BINARY_FOR_SETUP is not SET !")
endif ()

message("Enabling ASIO library (header-only) from local source ...")

set(ASIO_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/dependencies/asio/asio/include)

target_include_directories(${TARGET_BINARY_FOR_SETUP} PUBLIC ${ASIO_SOURCE_DIR})
target_compile_definitions(${TARGET_BINARY_FOR_SETUP} PUBLIC ASIO_STANDALONE ASIO_NO_EXCEPTIONS)

set(ASIO_ENABLED On)
