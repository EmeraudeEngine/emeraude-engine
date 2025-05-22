if ( NOT CPUFEATURES_ENABLED )
	#if ( EMERAUDE_USE_SYSTEM_LIBS )
	#	message("Enabling cpu_features library from system ...")

		# TODO ...
	#else ()
		message("Enabling cpu_features library from local source ...")

		if ( MSVC )
			target_link_libraries(${PROJECT_NAME} PRIVATE ${LOCAL_LIB_DIR}/lib/cpu_features.lib)
		else ()
			target_link_libraries(${PROJECT_NAME} PRIVATE ${LOCAL_LIB_DIR}/lib/libcpu_features.a)
		endif ()
	#endif ()

	set(CPUFEATURES_ENABLED On)
else ()
	message("The cpu_features library is already enabled.")
endif ()
