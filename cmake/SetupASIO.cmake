message("Enabling ASIO library (header-only) from local source ...")

set(ASIO_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/dependencies/asio/asio/include)

target_include_directories(${PROJECT_NAME} PUBLIC ${ASIO_SOURCE_DIR})
#target_compile_definitions(${PROJECT_NAME} PUBLIC ASIO_STANDALONE ASIO_NO_EXCEPTIONS)

set(ASIO_ENABLED On) # Complete the "libraries_config.hpp" file
