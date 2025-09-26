if ( NOT TARGET_BINARY_FOR_SETUP )
	message(FATAL_ERROR "TARGET_BINARY_FOR_SETUP is not SET !")
endif ()

if ( EMERAUDE_USE_SYSTEM_LIBS )
	message("Enabling OpenAL library from system ...")

	find_package(OpenAL REQUIRED)

	target_include_directories(${TARGET_BINARY_FOR_SETUP} PUBLIC ${OPENAL_INCLUDE_DIR})
	target_link_directories(${TARGET_BINARY_FOR_SETUP} PUBLIC ${OPENAL_LIBRARY_DIRS})
	target_link_libraries(${TARGET_BINARY_FOR_SETUP} PUBLIC ${OPENAL_LIBRARY})
else ()
	message("Enabling OpenAL-Soft library from local source ...")

	target_compile_definitions(${TARGET_BINARY_FOR_SETUP} PUBLIC AL_LIBTYPE_STATIC)

	if ( MSVC )
		target_link_libraries(${TARGET_BINARY_FOR_SETUP} PUBLIC winmm.lib)
		target_link_libraries(${TARGET_BINARY_FOR_SETUP} PUBLIC Avrt.lib)

		target_link_libraries(${TARGET_BINARY_FOR_SETUP} PUBLIC ${LOCAL_LIB_DIR}/lib/OpenAL32.lib)
	else ()
		if ( APPLE )
			target_link_libraries(${TARGET_BINARY_FOR_SETUP} PUBLIC "-framework CoreAudio")
			target_link_libraries(${TARGET_BINARY_FOR_SETUP} PUBLIC "-framework AudioToolbox")
		endif ()

		target_link_libraries(${TARGET_BINARY_FOR_SETUP} PUBLIC ${LOCAL_LIB_DIR}/lib/libopenal.a)
	endif ()
endif ()
