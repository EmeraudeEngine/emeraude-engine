if ( NOT TARGET_BINARY_FOR_SETUP )
	message(FATAL_ERROR "TARGET_BINARY_FOR_SETUP is not SET !")
endif ()

if ( EMERAUDE_USE_SYSTEM_LIBS )
	message("Enabling LibZib library from system ...")

	find_package(PkgConfig REQUIRED)

	pkg_check_modules(LIBZIP REQUIRED libzip)

	target_include_directories(${TARGET_BINARY_FOR_SETUP} PUBLIC ${LIBZIP_INCLUDE_DIRS})

	target_link_directories(${TARGET_BINARY_FOR_SETUP} PUBLIC ${LIBZIP_LIBRARY_DIRS})

	target_link_libraries(${TARGET_BINARY_FOR_SETUP} PRIVATE ${LIBZIP_LIBRARIES})
else ()
	message("Enabling LibZib library from local source ...")

	target_link_libraries(${TARGET_BINARY_FOR_SETUP} PRIVATE zip)
endif ()
