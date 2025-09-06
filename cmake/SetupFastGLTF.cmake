if ( NOT TARGET_BINARY_FOR_SETUP )
    message(FATAL_ERROR "TARGET_BINARY_FOR_SETUP is not SET !")
endif ()

message("Configuring FastGLTF library as sub-project ...")

set(FASTGLTF_COMPILE_AS_CPP20 ON)
set(FASTGLTF_ENABLE_CPP_MODULES OFF)
set(FASTGLTF_USE_STD_MODULE OFF)

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/dependencies/fastgltf EXCLUDE_FROM_ALL)

if ( UNIX )
    target_compile_options(fastgltf PRIVATE -fPIC)
endif ()

target_include_directories(${TARGET_BINARY_FOR_SETUP} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/dependencies/fastgltf/include)
target_link_libraries(${TARGET_BINARY_FOR_SETUP} PRIVATE fastgltf)
