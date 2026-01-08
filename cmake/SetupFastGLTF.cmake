if ( NOT TARGET_BINARY_FOR_SETUP )
	message(FATAL_ERROR "TARGET_BINARY_FOR_SETUP is not SET !")
endif ()

message("Enabling FastGLTF library from local precompiled source ...")

# NOTE: Headers are already included via ${LOCAL_LIB_DIR}/include in the main CMakeLists.txt

if ( MSVC )
	target_link_libraries(${TARGET_BINARY_FOR_SETUP} PRIVATE
		debug "${LOCAL_LIB_DIR}/lib/fastgltf.lib"
		optimized "${LOCAL_LIB_DIR}/lib/fastgltf.lib"
	)
else ()
	target_link_libraries(${TARGET_BINARY_FOR_SETUP} PRIVATE
		"${LOCAL_LIB_DIR}/lib/libfastgltf.a"
	)
endif ()