if ( NOT TARGET_BINARY_FOR_SETUP )
	message(FATAL_ERROR "TARGET_BINARY_FOR_SETUP is not SET !")
endif ()

if ( MSVC )
	message("Enabling SNDFile library from local binary ...")

	file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/${CMAKE_BUILD_TYPE})

	set(SNDFILE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/dependencies/libsndfile-1.2.2-win64)

	if ( NOT EXISTS ${CMAKE_BINARY_DIR}/${CMAKE_BUILD_TYPE}/sndfile.dll )
		file(
			COPY_FILE
			${SNDFILE_PATH}/bin/sndfile.dll
			${CMAKE_BINARY_DIR}/${CMAKE_BUILD_TYPE}/sndfile.dll
			ONLY_IF_DIFFERENT
		)
	endif ()

	target_include_directories(${TARGET_BINARY_FOR_SETUP} PUBLIC ${SNDFILE_PATH}/include)

	#target_link_directories(${TARGET_BINARY_FOR_SETUP} PUBLIC ${SNDFILE_PATH}/lib)

	target_link_libraries(${TARGET_BINARY_FOR_SETUP} PRIVATE ${SNDFILE_PATH}/lib/sndfile.lib)
else ()
	message("Enabling SNDFile library from system ...")

	find_package(PkgConfig REQUIRED)

	pkg_check_modules(SNDFILE REQUIRED sndfile)

	target_include_directories(${TARGET_BINARY_FOR_SETUP} PUBLIC ${SNDFILE_INCLUDE_DIRS})

	target_link_directories(${TARGET_BINARY_FOR_SETUP} PUBLIC ${SNDFILE_LIBRARY_DIRS})

	target_link_libraries(${TARGET_BINARY_FOR_SETUP} PRIVATE ${SNDFILE_LIBRARIES})
endif ()

set(LIBSNDFILE_ENABLED On)
