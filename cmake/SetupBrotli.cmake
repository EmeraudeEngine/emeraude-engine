if ( NOT TARGET_BINARY_FOR_SETUP )
	message(FATAL_ERROR "TARGET_BINARY_FOR_SETUP is not SET !")
endif ()

# NOTE: Brotli is unnecessary from system, freetype as system library will do the job.
if ( NOT EMERAUDE_USE_SYSTEM_LIBS )
	message("Enabling brotli library from local source ...")

	target_link_libraries(${TARGET_BINARY_FOR_SETUP} PRIVATE
		brotlidec
		brotlienc
		brotlicommon
	)
endif ()
