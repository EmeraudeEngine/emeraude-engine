if ( NOT OPENAL_ENABLED )
	if ( EMERAUDE_USE_SYSTEM_LIBS )
		message("Enabling OpenAL library from system ...")

		find_package(OpenAL REQUIRED)

		target_include_directories(${PROJECT_NAME} PRIVATE ${OPENAL_INCLUDE_DIR})
		target_link_directories(${PROJECT_NAME} PRIVATE ${OPENAL_LIBRARY_DIRS})
		target_link_libraries(${PROJECT_NAME} PRIVATE ${OPENAL_LIBRARY})
	else ()
		message("Enabling OpenAL-Soft library from local source ...")

		target_compile_definitions(${PROJECT_NAME} PRIVATE AL_LIBTYPE_STATIC)

		if ( MSVC )
			target_link_libraries(${PROJECT_NAME} PRIVATE winmm.lib)
			target_link_libraries(${PROJECT_NAME} PRIVATE Avrt.lib)

			target_link_libraries(${PROJECT_NAME} PRIVATE ${LOCAL_LIB_DIR}/lib/OpenAL32.lib)
		else ()
			if ( APPLE )
				target_link_libraries(${PROJECT_NAME} PRIVATE "-framework CoreAudio")
				target_link_libraries(${PROJECT_NAME} PRIVATE "-framework AudioToolbox")
			endif ()

			target_link_libraries(${PROJECT_NAME} PRIVATE ${LOCAL_LIB_DIR}/lib/libopenal.a)
		endif ()
	endif ()

	set(OPENAL_ENABLED On) # Complete the "emeraude_config.hpp"
else ()
	message("The OpenAL library is already enabled.")
endif ()
