if ( NOT TARGET_BINARY_FOR_SETUP )
	message(FATAL_ERROR "TARGET_BINARY_FOR_SETUP is not SET !")
endif ()

message("Enabling OpenAL-Soft library from local source ...")

target_compile_definitions(${TARGET_BINARY_FOR_SETUP} PUBLIC AL_LIBTYPE_STATIC)

if ( MSVC )
	target_link_libraries(${TARGET_BINARY_FOR_SETUP} PRIVATE winmm.lib)

	target_link_libraries(${TARGET_BINARY_FOR_SETUP} PRIVATE Avrt.lib)

	target_link_libraries(${TARGET_BINARY_FOR_SETUP} PRIVATE ${EMERAUDE_EXT_LIBS_PATH}/lib/OpenAL32.lib)
else ()
	if ( APPLE )
		target_link_libraries(${TARGET_BINARY_FOR_SETUP} PRIVATE "-framework CoreAudio -framework AudioToolbox")
	endif ()

	target_link_libraries(${TARGET_BINARY_FOR_SETUP} PRIVATE ${EMERAUDE_EXT_LIBS_PATH}/lib/libopenal.a)
endif ()
