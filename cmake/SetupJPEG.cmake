if ( NOT TARGET_BINARY_FOR_SETUP )
	message(FATAL_ERROR "TARGET_BINARY_FOR_SETUP is not SET !")
endif ()

if ( EMERAUDE_USE_SYSTEM_LIBS )
	message("Enabling LibJPEG-turbo library from system ...")

	find_package(PkgConfig REQUIRED)

	pkg_check_modules(JPEG REQUIRED libjpeg)

	target_include_directories(${TARGET_BINARY_FOR_SETUP} PUBLIC ${JPEG_INCLUDE_DIRS})

	target_link_directories(${TARGET_BINARY_FOR_SETUP} PUBLIC ${JPEG_LIBRARY_DIRS})

	target_link_libraries(${TARGET_BINARY_FOR_SETUP} PRIVATE ${JPEG_LIBRARIES})
else ()
	message("Enabling LibJPEG-turbo library from local source ...")

	if ( MSVC )
		target_link_libraries(${TARGET_BINARY_FOR_SETUP} PRIVATE jpeg-static)
	else ()
		target_link_libraries(${TARGET_BINARY_FOR_SETUP} PRIVATE jpeg)
	endif ()
endif ()
