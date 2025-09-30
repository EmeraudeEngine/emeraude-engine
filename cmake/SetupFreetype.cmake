if ( NOT TARGET_BINARY_FOR_SETUP )
	message(FATAL_ERROR "TARGET_BINARY_FOR_SETUP is not SET !")
endif ()

if ( EMERAUDE_USE_SYSTEM_LIBS )
	message("Enabling FreeType library from system ...")

	# NOTE: https://cmake.org/cmake/help/latest/module/FindFreetype.html
	find_package(Freetype REQUIRED)

	target_include_directories(${TARGET_BINARY_FOR_SETUP} PUBLIC ${FREETYPE_INCLUDE_DIRS})

	target_link_libraries(${TARGET_BINARY_FOR_SETUP} PRIVATE Freetype::Freetype)
else ()

	message("Enabling FreeType library from local source ...")

	target_include_directories(${TARGET_BINARY_FOR_SETUP} PUBLIC "${LOCAL_LIB_DIR}/include/freetype2")

	target_link_libraries(${TARGET_BINARY_FOR_SETUP} PRIVATE
		debug freetyped
		optimized freetype
	)
endif ()

if ( UNIX AND NOT APPLE )
	message("Enabling Fontconfig library from system ...")

	find_package(Fontconfig REQUIRED)

	target_include_directories(${TARGET_BINARY_FOR_SETUP} PUBLIC ${Fontconfig_INCLUDE_DIRS})

	target_link_libraries(${TARGET_BINARY_FOR_SETUP} PRIVATE ${Fontconfig_LIBRARIES})
	#target_compile_options(${TARGET_BINARY_FOR_SETUP} PUBLIC ${Fontconfig_COMPILE_OPTIONS})
endif ()
