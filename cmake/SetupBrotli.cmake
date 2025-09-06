if ( NOT TARGET_BINARY_FOR_SETUP )
	message(FATAL_ERROR "TARGET_BINARY_FOR_SETUP is not SET !")
endif ()

if ( EMERAUDE_USE_SYSTEM_LIBS )
	# NOTE: Brotli is unnecessary from system, freetype as system library will do the job.
else ()
	message("Enabling bzip2 library from local source ...")

	target_link_libraries(${TARGET_BINARY_FOR_SETUP} PUBLIC
		brotlidec
		brotlienc
		brotlicommon
	)
endif ()

set(BROTLI_ENABLED On)
