if ( NOT TARGET_BINARY_FOR_SETUP )
	message(FATAL_ERROR "TARGET_BINARY_FOR_SETUP is not SET !")
endif ()

if ( EMERAUDE_USE_SYSTEM_LIBS )
	message("Enabling bzip2 library from system ...")

	# NOTE: https://cmake.org/cmake/help/latest/module/FindBZip2.html
	find_package(BZip2 REQUIRED)

	target_include_directories(${TARGET_BINARY_FOR_SETUP} PUBLIC ${BZIP2_INCLUDE_DIRS})

	target_link_directories(${TARGET_BINARY_FOR_SETUP} PUBLIC ${BZIP2_LIBRARIES})

	target_link_libraries(${TARGET_BINARY_FOR_SETUP} PRIVATE BZip2::BZip2)
else ()
	message("Enabling bzip2 library from local source ...")

	if ( MSVC )
		target_link_libraries(${TARGET_BINARY_FOR_SETUP} PRIVATE ${LOCAL_LIB_DIR}/lib/bz2_static.lib)
	else ()
		target_link_libraries(${TARGET_BINARY_FOR_SETUP} PRIVATE ${LOCAL_LIB_DIR}/lib/libbz2_static.a)
	endif ()
endif ()
