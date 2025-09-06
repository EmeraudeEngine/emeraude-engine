if ( NOT TARGET_BINARY_FOR_SETUP )
	message(FATAL_ERROR "TARGET_BINARY_FOR_SETUP is not SET !")
endif ()

if ( EMERAUDE_USE_SYSTEM_LIBS )
	message("Enabling LibPNG library from system ...")

	find_package(PkgConfig REQUIRED)

	pkg_check_modules(PNG REQUIRED libpng)

	target_include_directories(${TARGET_BINARY_FOR_SETUP} PUBLIC ${PNG_INCLUDE_DIRS})

	target_link_directories(${TARGET_BINARY_FOR_SETUP} PUBLIC ${PNG_LIBRARY_DIRS})

	target_link_libraries(${TARGET_BINARY_FOR_SETUP} PRIVATE ${PNG_LIBRARIES})
else ()
	message("Enabling LibPNG library from local source ...")

	if ( MSVC )
		if ( CMAKE_BUILD_TYPE MATCHES Debug )
			target_link_libraries(${TARGET_BINARY_FOR_SETUP} PRIVATE ${LOCAL_LIB_DIR}/lib/libpng16_staticd.lib)
		else ()
			target_link_libraries(${TARGET_BINARY_FOR_SETUP} PRIVATE ${LOCAL_LIB_DIR}/lib/libpng16_static.lib)
		endif ()
	else ()
		if ( CMAKE_BUILD_TYPE MATCHES Debug )
			target_link_libraries(${TARGET_BINARY_FOR_SETUP} PRIVATE ${LOCAL_LIB_DIR}/lib/libpng16d.a)
		else ()
			target_link_libraries(${TARGET_BINARY_FOR_SETUP} PRIVATE ${LOCAL_LIB_DIR}/lib/libpng16.a)
		endif ()
	endif ()
endif ()
