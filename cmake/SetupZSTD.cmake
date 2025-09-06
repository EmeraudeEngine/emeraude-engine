if ( NOT TARGET_BINARY_FOR_SETUP )
	message(FATAL_ERROR "TARGET_BINARY_FOR_SETUP is not SET !")
endif ()

if ( EMERAUDE_USE_SYSTEM_LIBS )
	message("Enabling ZSTD library from system ...")

	find_package(PkgConfig REQUIRED)

	pkg_check_modules(ZSTD REQUIRED libzstd)

	target_include_directories(${TARGET_BINARY_FOR_SETUP} PUBLIC ${ZSTD_INCLUDE_DIRS})

	target_link_directories(${TARGET_BINARY_FOR_SETUP} PUBLIC ${ZSTD_LIBRARY_DIRS})

	target_link_libraries(${TARGET_BINARY_FOR_SETUP} PRIVATE ${ZSTD_LIBRARIES})
else ()
	message("Enabling ZSTD library from local source ...")

	if ( MSVC )
		target_link_libraries(${TARGET_BINARY_FOR_SETUP} PRIVATE ${LOCAL_LIB_DIR}/lib/zstd_static.lib)
	else ()
		target_link_libraries(${TARGET_BINARY_FOR_SETUP} PRIVATE ${LOCAL_LIB_DIR}/lib/libzstd.a)
	endif ()
endif ()
