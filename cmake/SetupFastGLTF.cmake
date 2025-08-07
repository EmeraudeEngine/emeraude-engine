if ( NOT FASTGLTF_ENABLED )
    message("Configuring FastGLTF library as sub-project ...")

    set(FASTGLTF_COMPILE_AS_CPP20 ON)
    set(FASTGLTF_ENABLE_CPP_MODULES OFF)
    set(FASTGLTF_USE_STD_MODULE OFF)

    add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/dependencies/fastgltf EXCLUDE_FROM_ALL)

    if ( UNIX )
        target_compile_options(fastgltf PRIVATE -fPIC)
    endif ()

    target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/dependencies/fastgltf/include)
    target_link_libraries(${PROJECT_NAME} PUBLIC fastgltf)

    set(FASTGLTF_ENABLED On)
else ()
    message("The FastGLTF library is already enabled.")
endif ()
