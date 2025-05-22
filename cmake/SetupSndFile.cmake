if ( NOT SNDFILE_ENABLED )
	if ( MSVC )
		message("Enabling SNDFile library from local binary ...")

		file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/${CMAKE_BUILD_TYPE})

		set(SNDFILE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/dependencies/libsndfile-1.2.2-win64)

		if ( NOT EXISTS ${CMAKE_BINARY_DIR}/${CMAKE_BUILD_TYPE}/sndfile.dll )
			file(
				COPY_FILE
				${SNDFILE_PATH_DLL}/bin/sndfile.dll
				${CMAKE_BINARY_DIR}/${CMAKE_BUILD_TYPE}/sndfile.dll
				ONLY_IF_DIFFERENT
			)
		endif ()

		target_include_directories(${BINARY_TARGET_NAME} PRIVATE ${SNDFILE_PATH}/include)
		#target_link_directories(${BINARY_TARGET_NAME} PRIVATE ${SNDFILE_PATH}/lib)
		target_link_libraries(${BINARY_TARGET_NAME} PRIVATE ${SNDFILE_PATH}/lib/sndfile.lib)
	else ()
		message("Enabling SNDFile library from system ...")

		find_package(PkgConfig REQUIRED)

		pkg_check_modules(SNDFILE REQUIRED sndfile)

		target_include_directories(${BINARY_TARGET_NAME} PRIVATE ${SNDFILE_INCLUDE_DIRS})
		target_link_directories(${BINARY_TARGET_NAME} PRIVATE ${SNDFILE_LIBRARY_DIRS})
		target_link_libraries(${BINARY_TARGET_NAME} PRIVATE ${SNDFILE_LIBRARIES})
	endif ()

	set(SNDFILE_ENABLED On)
else ()
	message("The SNDFile library is already enabled.")
endif ()
