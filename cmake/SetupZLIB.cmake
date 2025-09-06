if ( NOT TARGET_BINARY_FOR_SETUP )
	message(FATAL_ERROR "TARGET_BINARY_FOR_SETUP is not SET !")
endif ()

if ( EMERAUDE_USE_SYSTEM_LIBS )
	message("Enabling zlib library from system ...")

	find_package(PkgConfig REQUIRED)

	pkg_check_modules(ZLIB REQUIRED zlib)

	target_include_directories(${TARGET_BINARY_FOR_SETUP} PUBLIC ${ZLIB_INCLUDE_DIRS})

	target_link_directories(${TARGET_BINARY_FOR_SETUP} PUBLIC ${ZLIB_LIBRARY_DIRS})

	target_link_libraries(${TARGET_BINARY_FOR_SETUP} PRIVATE ${ZLIB_LIBRARIES})
else ()
	message("Enabling zlib library from local source ...")

	if ( MSVC )
		if ( CMAKE_BUILD_TYPE MATCHES Debug )
			target_link_libraries(${TARGET_BINARY_FOR_SETUP} PRIVATE ${LOCAL_LIB_DIR}/lib/zlibstaticd.lib) # Change to 'zsd' when new version of zlib will be released
		else ()
			target_link_libraries(${TARGET_BINARY_FOR_SETUP} PRIVATE ${LOCAL_LIB_DIR}/lib/zlibstatic.lib) # Change to 'zs' when new version of zlib will be released
		endif ()
	else ()
		target_link_libraries(${TARGET_BINARY_FOR_SETUP} PRIVATE ${LOCAL_LIB_DIR}/lib/libz.a) # Change to 'z' when new version of zlib will be released
	endif ()
endif ()
