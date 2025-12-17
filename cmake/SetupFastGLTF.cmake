if ( NOT TARGET_BINARY_FOR_SETUP )
    message(FATAL_ERROR "TARGET_BINARY_FOR_SETUP is not SET !")
endif ()

message("Configuring FastGLTF library as sub-project ...")

set(FASTGLTF_COMPILE_AS_CPP20 ON)
set(FASTGLTF_ENABLE_CPP_MODULES OFF)
set(FASTGLTF_USE_STD_MODULE OFF)

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/dependencies/fastgltf EXCLUDE_FROM_ALL)

# Enable Position Independent Code for shared library compatibility
set_target_properties(fastgltf PROPERTIES POSITION_INDEPENDENT_CODE ON)

if ( EMERAUDE_DISABLE_PARANOID_COMPILATION )
    target_include_directories(${TARGET_BINARY_FOR_SETUP} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/dependencies/fastgltf/include)
else ()
    # Mark VMA include directory as SYSTEM to suppress warnings
    target_include_directories(${TARGET_BINARY_FOR_SETUP} SYSTEM PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/dependencies/fastgltf/include)
endif ()

target_link_libraries(${TARGET_BINARY_FOR_SETUP} PRIVATE fastgltf)
