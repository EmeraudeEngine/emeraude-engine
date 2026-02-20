# cmake/BuildRenderDocPython.cmake
#
# Builds the RenderDoc Python module (renderdoc.so) from the submodule.
# This enables programmatic analysis of .rdc GPU captures via:
#   import renderdoc as rd
#
# Linux only. Requires: python3-dev, swig, bison, libxcb-keysyms1-dev
#
# Usage:
#   cmake -P cmake/BuildRenderDocPython.cmake

cmake_minimum_required(VERSION 3.20)

# --- Paths ---

if ( NOT DEFINED RENDERDOC_SOURCE_DIR )
	get_filename_component(RENDERDOC_SOURCE_DIR "${CMAKE_CURRENT_LIST_DIR}/../dependencies/renderdoc" ABSOLUTE)
endif ()

if ( NOT DEFINED RENDERDOC_BUILD_DIR )
	set(RENDERDOC_BUILD_DIR "${RENDERDOC_SOURCE_DIR}/build")
endif ()

set(RENDERDOC_PYTHON_MODULE "${RENDERDOC_BUILD_DIR}/lib/renderdoc.so")

# --- Guard: already built ---

if ( EXISTS "${RENDERDOC_PYTHON_MODULE}" )
	message(STATUS "[RenderDoc] Python module already built: ${RENDERDOC_PYTHON_MODULE}")
	return()
endif ()

# --- Guard: submodule present ---

if ( NOT EXISTS "${RENDERDOC_SOURCE_DIR}/CMakeLists.txt" )
	message(FATAL_ERROR "[RenderDoc] Submodule not found at ${RENDERDOC_SOURCE_DIR}. Run: git submodule update --init dependencies/renderdoc")
endif ()

# --- Guard: Linux only ---

if ( NOT CMAKE_HOST_UNIX )
	message(STATUS "[RenderDoc] Python module build is Linux-only. Skipping.")
	return()
endif ()

if ( CMAKE_HOST_APPLE )
	message(STATUS "[RenderDoc] Python module build is Linux-only (not macOS). Skipping.")
	return()
endif ()

# --- Find system Python3 (skip virtualenvs) ---

find_program(SYSTEM_PYTHON3 python3 PATHS /usr/bin NO_DEFAULT_PATH)

if ( NOT SYSTEM_PYTHON3 )
	find_program(SYSTEM_PYTHON3 python3)
endif ()

if ( NOT SYSTEM_PYTHON3 )
	message(WARNING "[RenderDoc] python3 not found. Install: sudo apt install python3-dev")
	return()
endif ()

# Verify python3-dev is installed (check for Python.h)
execute_process(
	COMMAND ${SYSTEM_PYTHON3} -c "import sysconfig; print(sysconfig.get_path('include'))"
	OUTPUT_VARIABLE PYTHON3_INCLUDE_DIR
	OUTPUT_STRIP_TRAILING_WHITESPACE
	ERROR_QUIET
)

if ( NOT EXISTS "${PYTHON3_INCLUDE_DIR}/Python.h" )
	message(WARNING "[RenderDoc] python3-dev not found. Install: sudo apt install python3-dev")
	return()
endif ()

# --- Check other prerequisites ---

find_program(SWIG_EXE swig)
find_program(BISON_EXE bison)

if ( NOT SWIG_EXE )
	message(WARNING "[RenderDoc] swig not found. Install: sudo apt install swig")
	return()
endif ()

if ( NOT BISON_EXE )
	message(WARNING "[RenderDoc] bison not found. Install: sudo apt install bison")
	return()
endif ()

message(STATUS "[RenderDoc] Building Python module (this takes a few minutes) ...")
message(STATUS "[RenderDoc]   Python: ${SYSTEM_PYTHON3}")
message(STATUS "[RenderDoc]   Include: ${PYTHON3_INCLUDE_DIR}")

# ============================================================
# Step 1: Configure RenderDoc
# ============================================================

execute_process(
	COMMAND ${CMAKE_COMMAND}
		-B "${RENDERDOC_BUILD_DIR}"
		-DCMAKE_BUILD_TYPE=Release
		-DENABLE_QRENDERDOC=OFF
		-DENABLE_RENDERDOCCMD=OFF
		-DENABLE_PYRENDERDOC=ON
		-DPython3_EXECUTABLE=${SYSTEM_PYTHON3}
		-DPython3_INCLUDE_DIR=${PYTHON3_INCLUDE_DIR}
	WORKING_DIRECTORY "${RENDERDOC_SOURCE_DIR}"
	RESULT_VARIABLE _result
	OUTPUT_QUIET ERROR_QUIET
)

if ( NOT _result EQUAL 0 )
	message(WARNING "[RenderDoc] CMake configure failed (exit ${_result}).")
	return()
endif ()

message(STATUS "[RenderDoc] Step 1/5: Configured.")

# ============================================================
# Step 2: Build librenderdoc.so
# ============================================================

execute_process(
	COMMAND ${CMAKE_COMMAND} --build "${RENDERDOC_BUILD_DIR}" --target renderdoc -j
	RESULT_VARIABLE _result
	OUTPUT_QUIET ERROR_QUIET
)

if ( NOT _result EQUAL 0 )
	message(WARNING "[RenderDoc] librenderdoc.so build failed.")
	return()
endif ()

message(STATUS "[RenderDoc] Step 2/5: librenderdoc.so built.")

# ============================================================
# Step 3: Build PCRE1 (RenderDoc's custom SWIG needs it)
#
# RenderDoc downloads pcre-8.45 via ExternalProject.
# Its autotools build is fragile, so we trigger the download
# then build with CMake ourselves.
# ============================================================

set(PCRE_STAMP_DIR "${RENDERDOC_BUILD_DIR}/qrenderdoc/local_pcre-prefix/src/local_pcre-stamp")
set(PCRE_SOURCE_DIR "${RENDERDOC_BUILD_DIR}/qrenderdoc/local_pcre-prefix/src/local_pcre")
set(PCRE_BUILD_DIR "${RENDERDOC_BUILD_DIR}/qrenderdoc/local_pcre-prefix/src/local_pcre-build")

if ( NOT EXISTS "${RENDERDOC_BUILD_DIR}/lib/libpcre.a" )
	# Trigger the ExternalProject download (the build will fail, that's expected)
	execute_process(
		COMMAND ${CMAKE_COMMAND} --build "${RENDERDOC_BUILD_DIR}" --target local_pcre -j
		OUTPUT_QUIET ERROR_QUIET
	)

	if ( NOT EXISTS "${PCRE_SOURCE_DIR}/CMakeLists.txt" )
		message(WARNING "[RenderDoc] PCRE1 source download failed.")
		return()
	endif ()

	# Build PCRE1 with CMake (reliable, unlike autotools)
	execute_process(
		COMMAND ${CMAKE_COMMAND}
			-B "${PCRE_BUILD_DIR}"
			-DCMAKE_POSITION_INDEPENDENT_CODE=ON
			-DCMAKE_INSTALL_PREFIX=${RENDERDOC_BUILD_DIR}
		WORKING_DIRECTORY "${PCRE_SOURCE_DIR}"
		OUTPUT_QUIET ERROR_QUIET
	)

	execute_process(
		COMMAND ${CMAKE_COMMAND} --build "${PCRE_BUILD_DIR}" -j
		RESULT_VARIABLE _result
		OUTPUT_QUIET ERROR_QUIET
	)

	if ( NOT _result EQUAL 0 )
		message(WARNING "[RenderDoc] PCRE1 build failed.")
		return()
	endif ()

	execute_process(
		COMMAND ${CMAKE_COMMAND} --install "${PCRE_BUILD_DIR}" --prefix "${RENDERDOC_BUILD_DIR}"
		OUTPUT_QUIET ERROR_QUIET
	)

	# Fix pcre-config: it hardcodes prefix=/usr/local
	set(PCRE_CONFIG_FILE "${RENDERDOC_BUILD_DIR}/bin/pcre-config")
	if ( EXISTS "${PCRE_CONFIG_FILE}" )
		file(READ "${PCRE_CONFIG_FILE}" _content)
		string(REPLACE "prefix=/usr/local" "prefix=${RENDERDOC_BUILD_DIR}" _content "${_content}")
		file(WRITE "${PCRE_CONFIG_FILE}" "${_content}")
	endif ()
endif ()

# Mark ExternalProject stamps as done
file(TOUCH "${PCRE_STAMP_DIR}/local_pcre-build")
file(TOUCH "${PCRE_STAMP_DIR}/local_pcre-install")

message(STATUS "[RenderDoc] Step 3/5: PCRE1 ready.")

# ============================================================
# Step 4: Build custom SWIG fork
#
# RenderDoc uses a modified SWIG that supports scoped enums.
# It downloads from GitHub and builds with autotools.
# ============================================================

set(SWIG_STAMP_DIR "${RENDERDOC_BUILD_DIR}/qrenderdoc/custom_swig-prefix/src/custom_swig-stamp")
set(SWIG_SOURCE_DIR "${RENDERDOC_BUILD_DIR}/qrenderdoc/custom_swig-prefix/src/custom_swig")
set(SWIG_BINARY "${RENDERDOC_BUILD_DIR}/bin/swig")

if ( NOT EXISTS "${SWIG_BINARY}" )
	# Trigger the ExternalProject download (build may fail, that's expected)
	execute_process(
		COMMAND ${CMAKE_COMMAND} --build "${RENDERDOC_BUILD_DIR}" --target custom_swig -j
		OUTPUT_QUIET ERROR_QUIET
	)

	if ( NOT EXISTS "${SWIG_SOURCE_DIR}/configure" )
		message(WARNING "[RenderDoc] Custom SWIG source download failed.")
		return()
	endif ()

	# Configure with correct pcre prefix
	execute_process(
		COMMAND ${CMAKE_COMMAND} -E env "CFLAGS=-fPIC" "CXXFLAGS=-fPIC"
			./configure
			--with-pcre=yes
			--with-pcre-prefix=${RENDERDOC_BUILD_DIR}
			--prefix=${RENDERDOC_BUILD_DIR}
			--disable-shared
		WORKING_DIRECTORY "${SWIG_SOURCE_DIR}"
		OUTPUT_QUIET ERROR_QUIET
	)

	# Build
	execute_process(
		COMMAND make -j
		WORKING_DIRECTORY "${SWIG_SOURCE_DIR}"
		RESULT_VARIABLE _result
		OUTPUT_QUIET ERROR_QUIET
	)

	if ( NOT _result EQUAL 0 )
		message(WARNING "[RenderDoc] Custom SWIG build failed.")
		return()
	endif ()

	# Install (copies swig binary + lib files)
	execute_process(
		COMMAND make install
		WORKING_DIRECTORY "${SWIG_SOURCE_DIR}"
		OUTPUT_QUIET ERROR_QUIET
	)
endif ()

if ( NOT EXISTS "${SWIG_BINARY}" )
	message(WARNING "[RenderDoc] SWIG binary not produced at ${SWIG_BINARY}")
	return()
endif ()

# Mark stamps
file(TOUCH "${SWIG_STAMP_DIR}/custom_swig-build")
file(TOUCH "${SWIG_STAMP_DIR}/custom_swig-install")

message(STATUS "[RenderDoc] Step 4/5: Custom SWIG ready.")

# ============================================================
# Step 5: Generate SWIG bindings + build renderdoc.so
# ============================================================

execute_process(
	COMMAND ${CMAKE_COMMAND} --build "${RENDERDOC_BUILD_DIR}" -j
	RESULT_VARIABLE _result
	OUTPUT_QUIET ERROR_QUIET
)

if ( EXISTS "${RENDERDOC_PYTHON_MODULE}" )
	message(STATUS "[RenderDoc] Step 5/5: renderdoc.so built successfully.")
	message(STATUS "[RenderDoc] Module: ${RENDERDOC_PYTHON_MODULE}")
else ()
	message(WARNING "[RenderDoc] Build completed (exit ${_result}) but renderdoc.so not found.")
	message(WARNING "[RenderDoc] Check logs: cmake --build ${RENDERDOC_BUILD_DIR} -j 2>&1")
endif ()
