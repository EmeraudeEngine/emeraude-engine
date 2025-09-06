if ( NOT TARGET_BINARY_FOR_SETUP )
	message(FATAL_ERROR "TARGET_BINARY_FOR_SETUP is not SET !")
endif ()

if ( EMERAUDE_USE_SYSTEM_LIBS )
	# NOTE: Harfbuzz is unnecessary from system, freetype as system library will do the job.
else ()
	message("Enabling Harfbuzz library from local source ...")

	target_link_libraries(${TARGET_BINARY_FOR_SETUP} PUBLIC
		harfbuzz
		harfbuzz-subset
	)
endif ()

set(HARFBUZZ_ENABLED On)
