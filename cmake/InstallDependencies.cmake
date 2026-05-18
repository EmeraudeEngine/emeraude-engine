message("Installing external dependencies ...")

# Developer bypass: if LOCAL_LIB_DIR is a symbolic link (POSIX) or a directory
# junction (Windows `mklink /J` / `mklink /D`), trust the local setup — e.g.
# a developer pointing to a locally-built ext-deps-generator output — and
# skip the download + extraction entirely.
if ( IS_SYMLINK ${LOCAL_LIB_DIR} AND IS_DIRECTORY ${LOCAL_LIB_DIR} )
	message("External dependencies directory '${LOCAL_LIB_DIR}' is a symbolic link — bypassing download and extraction.")
	return ()
endif ()

# Get the right build type name.
if ( CMAKE_BUILD_TYPE MATCHES "Release|RelWithDebInfo|MinSizeRel" )
	set(EXTERNAL_DEPENDENCIES_BUILD_TYPE Release)
else ()
	set(EXTERNAL_DEPENDENCIES_TYPE Debug)
endif ()

# Get the right archive name.
if ( UNIX AND NOT APPLE )
	find_program(LSB_RELEASE_EXEC lsb_release REQUIRED)

	execute_process(COMMAND ${LSB_RELEASE_EXEC} -is OUTPUT_VARIABLE LSB_RELEASE_ID_SHORT OUTPUT_STRIP_TRAILING_WHITESPACE)

	set(EXTERNAL_DEPENDENCIES_VERSION "010" CACHE STRING "The external dependencies version to download for Linux.")
	set(EXTERNAL_DEPENDENCIES_FILENAME linux-${LSB_RELEASE_ID_SHORT}.x86_64-${EXTERNAL_DEPENDENCIES_BUILD_TYPE}-${EXTERNAL_DEPENDENCIES_VERSION}.zip)
elseif ( APPLE )
	set(EXTERNAL_DEPENDENCIES_VERSION "010" CACHE STRING "The external dependencies version to download for macOS.")
	set(EXTERNAL_DEPENDENCIES_FILENAME mac.${CMAKE_OSX_ARCHITECTURES}-${EXTERNAL_DEPENDENCIES_BUILD_TYPE}-${EXTERNAL_DEPENDENCIES_VERSION}.zip)
elseif ( MSVC )
	set(EXTERNAL_DEPENDENCIES_VERSION "010" CACHE STRING "The external dependencies version to download for Windows.")

	if ( EMERAUDE_USE_STATIC_RUNTIME )
		set(EXTERNAL_DEPENDENCIES_FILENAME windows.x86_64-${EXTERNAL_DEPENDENCIES_BUILD_TYPE}-MT-${EXTERNAL_DEPENDENCIES_VERSION}.zip)
	else ()
		set(EXTERNAL_DEPENDENCIES_FILENAME windows.x86_64-${EXTERNAL_DEPENDENCIES_BUILD_TYPE}-MD-${EXTERNAL_DEPENDENCIES_VERSION}.zip)
	endif ()
else ()
	message(FATAL_ERROR "Unable to detect the OS to download external dependencies !")
endif ()

# Resolve URL and local paths.
# Archives are hosted as GitHub Release assets, tagged 'v<PCK_VERSION>'.
set(EXTERNAL_DEPENDENCIES_URL "https://github.com/EmeraudeEngine/ext-deps-generator/releases/download/v${EXTERNAL_DEPENDENCIES_VERSION}/${EXTERNAL_DEPENDENCIES_FILENAME}")
set(EXTERNAL_DEPENDENCIES_DIR "${CMAKE_CURRENT_SOURCE_DIR}/dependencies")
set(EXTERNAL_DEPENDENCIES_PATH "${EXTERNAL_DEPENDENCIES_DIR}/${EXTERNAL_DEPENDENCIES_FILENAME}")

# Download the archive if it isn't cached yet.
# A fresh download forces re-extraction afterwards (so version bumps overwrite the old extracted directory).
set(EXTERNAL_DEPENDENCIES_FRESH_DOWNLOAD FALSE)

if ( NOT EXISTS ${EXTERNAL_DEPENDENCIES_PATH} )
	message("External dependencies archive '${EXTERNAL_DEPENDENCIES_FILENAME}' is not present ! Downloading it from ${EXTERNAL_DEPENDENCIES_URL} ...")

	file(MAKE_DIRECTORY ${EXTERNAL_DEPENDENCIES_DIR})

	file(DOWNLOAD
		${EXTERNAL_DEPENDENCIES_URL}
		${EXTERNAL_DEPENDENCIES_PATH}
		SHOW_PROGRESS
		STATUS EXTERNAL_DEPENDENCIES_DOWNLOAD_STATUS
		LOG EXTERNAL_DEPENDENCIES_DOWNLOAD_LOG
	)

	list(GET EXTERNAL_DEPENDENCIES_DOWNLOAD_STATUS 0 EXTERNAL_DEPENDENCIES_DOWNLOAD_CODE)
	list(GET EXTERNAL_DEPENDENCIES_DOWNLOAD_STATUS 1 EXTERNAL_DEPENDENCIES_DOWNLOAD_MESSAGE)

	if ( NOT EXTERNAL_DEPENDENCIES_DOWNLOAD_CODE EQUAL 0 )
		# Remove the (potentially partial / HTML-error) file so we don't try to extract garbage.
		file(REMOVE ${EXTERNAL_DEPENDENCIES_PATH})

		message(FATAL_ERROR
			"Failed to download external dependencies.\n"
			"  URL:    ${EXTERNAL_DEPENDENCIES_URL}\n"
			"  Status: ${EXTERNAL_DEPENDENCIES_DOWNLOAD_CODE} (${EXTERNAL_DEPENDENCIES_DOWNLOAD_MESSAGE})\n"
			"  Log:\n${EXTERNAL_DEPENDENCIES_DOWNLOAD_LOG}"
		)
	endif ()

	set(EXTERNAL_DEPENDENCIES_FRESH_DOWNLOAD TRUE)
else ()
	message("External dependencies archive '${EXTERNAL_DEPENDENCIES_FILENAME}' is present !")
endif ()

# Extract the archive when:
#   - the extracted directory doesn't exist yet, OR
#   - we just freshly downloaded the archive (force overwrite — version bump or replaced asset).
if ( EXTERNAL_DEPENDENCIES_FRESH_DOWNLOAD OR NOT EXISTS ${LOCAL_LIB_DIR} )
	if ( EXISTS ${LOCAL_LIB_DIR} )
		message("Removing previous extracted directory '${LOCAL_LIB_DIR}' before re-extracting ...")
		file(REMOVE_RECURSE ${LOCAL_LIB_DIR})
	endif ()

	message("Extracting archive ${EXTERNAL_DEPENDENCIES_PATH} ...")

	file(ARCHIVE_EXTRACT
		INPUT ${EXTERNAL_DEPENDENCIES_PATH}
		DESTINATION ${EXTERNAL_DEPENDENCIES_DIR}
		VERBOSE
	)

	if ( NOT EXISTS ${LOCAL_LIB_DIR} )
		message(FATAL_ERROR
			"Archive extraction did not produce the expected directory.\n"
			"  Archive:  ${EXTERNAL_DEPENDENCIES_PATH}\n"
			"  Expected: ${LOCAL_LIB_DIR}\n"
			"The archive contents may not match the engine's directory convention."
		)
	endif ()
else ()
	message("External dependencies binaries present !")
endif ()
