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

	target_include_directories(${TARGET_BINARY_FOR_SETUP} PUBLIC ${LOCAL_LIB_DIR}/include/freetype2)

	if ( CMAKE_BUILD_TYPE MATCHES Debug )
		target_link_libraries(${TARGET_BINARY_FOR_SETUP} PRIVATE freetyped)
	else ()
		target_link_libraries(${TARGET_BINARY_FOR_SETUP} PRIVATE freetype)
	endif ()
endif ()

if ( UNIX AND NOT APPLE )
	message("Enabling Fontconfig library from system ...")

	find_package(Fontconfig REQUIRED)

	target_include_directories(${TARGET_BINARY_FOR_SETUP} PUBLIC ${Fontconfig_INCLUDE_DIRS})

	target_link_libraries(${TARGET_BINARY_FOR_SETUP} PRIVATE ${Fontconfig_LIBRARIES})
endif ()
