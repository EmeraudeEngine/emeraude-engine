if ( NOT TARGET_BINARY_FOR_SETUP )
	message(FATAL_ERROR "TARGET_BINARY_FOR_SETUP is not SET !")
endif ()

if ( EMERAUDE_USE_SYSTEM_LIBS )
	message("Enabling TagLib library from system ...")

	find_package(PkgConfig REQUIRED)

	pkg_check_modules(TAGLIB REQUIRED taglib)

	target_include_directories(${TARGET_BINARY_FOR_SETUP} PUBLIC ${TAGLIB_INCLUDE_DIRS})

	target_link_directories(${TARGET_BINARY_FOR_SETUP} PUBLIC ${TAGLIB_LIBRARY_DIRS})

	target_link_libraries(${TARGET_BINARY_FOR_SETUP} PRIVATE ${TAGLIB_LIBRARIES})
else ()
	message("Enabling TagLib library from local source ...")

	if ( MSVC )
		target_compile_definitions(${TARGET_BINARY_FOR_SETUP} PRIVATE TAGLIB_STATIC)

		target_link_libraries(${TARGET_BINARY_FOR_SETUP} PRIVATE ${LOCAL_LIB_DIR}/lib/tag.lib)
	else ()
		target_link_libraries(${TARGET_BINARY_FOR_SETUP} PRIVATE ${LOCAL_LIB_DIR}/lib/libtag.a)
	endif ()
endif ()
