if ( NOT TARGET_BINARY_FOR_SETUP )
	message(FATAL_ERROR "TARGET_BINARY_FOR_SETUP is not SET !")
endif ()

if ( EMERAUDE_USE_SYSTEM_LIBS )
	message("Enabling SampleRate library from system ...")

	find_package(PkgConfig REQUIRED)

	pkg_check_modules(SAMPLERATE REQUIRED samplerate)

	target_include_directories(${TARGET_BINARY_FOR_SETUP} PUBLIC ${SAMPLERATE_INCLUDE_DIRS})

	target_link_directories(${TARGET_BINARY_FOR_SETUP} PUBLIC ${SAMPLERATE_LIBRARY_DIRS})

	target_link_libraries(${TARGET_BINARY_FOR_SETUP} PRIVATE ${SAMPLERATE_LIBRARIES})
else ()
	message("Enabling SampleRate library from local source ...")

	if ( MSVC )
		target_link_libraries(${TARGET_BINARY_FOR_SETUP} PRIVATE ${LOCAL_LIB_DIR}/lib/samplerate.lib)
	else ()
		target_link_libraries(${TARGET_BINARY_FOR_SETUP} PRIVATE ${LOCAL_LIB_DIR}/lib/libsamplerate.a)
	endif ()
endif ()
