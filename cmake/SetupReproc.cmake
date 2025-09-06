if ( NOT TARGET_BINARY_FOR_SETUP )
	message(FATAL_ERROR "TARGET_BINARY_FOR_SETUP is not SET !")
endif ()

message("Configuring reproc/reproc++ library as sub-project ...")

set(REPROC++ ON)
set(REPROC_MULTITHREADED ON)

set(REPROC_DEVELOP OFF)
set(REPROC_TEST OFF)
set(REPROC_EXAMPLES OFF)
set(REPROC_WARNINGS ON)
set(REPROC_TIDY OFF)
set(REPROC_SANITIZERS OFF)
set(REPROC_WARNINGS_AS_ERRORS OFF)
set(REPROC_OBJECT_LIBRARIES ON)

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/dependencies/reproc EXCLUDE_FROM_ALL)

if ( UNIX )
	target_compile_options(reproc PRIVATE -fPIC)
	target_compile_options(reproc++ PRIVATE -fPIC)
endif ()

target_include_directories(${TARGET_BINARY_FOR_SETUP} PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/dependencies/reproc/reproc/include
    ${CMAKE_CURRENT_SOURCE_DIR}/dependencies/reproc/reproc++/include
)
target_link_libraries(${TARGET_BINARY_FOR_SETUP} PRIVATE reproc reproc++)
