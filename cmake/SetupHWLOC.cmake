if ( NOT TARGET_BINARY_FOR_SETUP )
	message(FATAL_ERROR "TARGET_BINARY_FOR_SETUP is not SET !")
endif ()

if ( EMERAUDE_USE_SYSTEM_LIBS )
	message("Enabling HWLOC library from system ...")

	find_package(PkgConfig REQUIRED)

	pkg_check_modules(HWLOC REQUIRED hwloc)

	target_include_directories(${TARGET_BINARY_FOR_SETUP} PUBLIC ${HWLOC_INCLUDE_DIRS})

	target_link_directories(${TARGET_BINARY_FOR_SETUP} PUBLIC ${HWLOC_LIBRARY_DIRS})

	target_link_libraries(${TARGET_BINARY_FOR_SETUP} PRIVATE ${HWLOC_LIBRARIES})
else ()
	message("Enabling HWLOC library from local source ...")

	if ( MSVC )
		target_link_libraries(${TARGET_BINARY_FOR_SETUP} PRIVATE ${LOCAL_LIB_DIR}/lib/hwloc.lib)
	else ()

		target_link_libraries(${TARGET_BINARY_FOR_SETUP} PRIVATE ${LOCAL_LIB_DIR}/lib/libhwloc.a)

		if ( APPLE )
			target_link_libraries(${TARGET_BINARY_FOR_SETUP} PRIVATE "-framework CoreFoundation -framework IOKit")
		endif ()
	endif ()
endif ()
