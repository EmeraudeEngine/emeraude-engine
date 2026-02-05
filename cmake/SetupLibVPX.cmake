if ( NOT TARGET_BINARY_FOR_SETUP )
	message(FATAL_ERROR "TARGET_BINARY_FOR_SETUP is not SET !")
endif ()

if ( EMERAUDE_USE_SYSTEM_LIBS )
	message("Enabling libvpx library from system ...")

	find_package(PkgConfig REQUIRED)

	pkg_check_modules(VPX REQUIRED vpx)

	target_include_directories(${TARGET_BINARY_FOR_SETUP} PUBLIC ${VPX_INCLUDE_DIRS})

	target_link_directories(${TARGET_BINARY_FOR_SETUP} PUBLIC ${VPX_LIBRARY_DIRS})

	target_link_libraries(${TARGET_BINARY_FOR_SETUP} PRIVATE ${VPX_LIBRARIES})
else ()
	message("Enabling libvpx library from local source ...")

	if ( MSVC )
		if ( EMERAUDE_USE_STATIC_RUNTIME )
			target_link_libraries(${TARGET_BINARY_FOR_SETUP} PRIVATE ${LOCAL_LIB_DIR}/lib/vpxmt.lib)
		else ()
			target_link_libraries(${TARGET_BINARY_FOR_SETUP} PRIVATE ${LOCAL_LIB_DIR}/lib/vpxmd.lib)
		endif ()
	else ()
		target_link_libraries(${TARGET_BINARY_FOR_SETUP} PRIVATE ${LOCAL_LIB_DIR}/lib/libvpx.a)
	endif ()
endif ()

set(EMERAUDE_VIDEO_RECORDING_ENABLED On)
