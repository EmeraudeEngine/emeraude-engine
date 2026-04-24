if ( NOT TARGET_BINARY_FOR_SETUP )
	message(FATAL_ERROR "TARGET_BINARY_FOR_SETUP is not SET !")
endif ()

message("Configuring ufbx FBX parsing library ...")

set(UFBX_DIR ${CMAKE_CURRENT_SOURCE_DIR}/dependencies/ufbx)

# Build a static library from the single translation unit `ufbx.c`.
# ufbx is a zero-dependency C library for parsing FBX files.
# Pinned to v0.21.3.
add_library(ufbx STATIC
	${UFBX_DIR}/ufbx.c
)

set_target_properties(ufbx PROPERTIES
	C_STANDARD 99
	C_STANDARD_REQUIRED ON
	POSITION_INDEPENDENT_CODE ON
)

target_include_directories(ufbx SYSTEM PUBLIC ${UFBX_DIR})

# Suppress warnings from third-party code (compiled with -Werror in the engine).
# Optimization flags are inherited from the build type (Release/Debug) —
# do not force them here to avoid MSVC flag conflicts (e.g., /RTC1 vs /O2).
if ( NOT MSVC )
	target_compile_options(ufbx PRIVATE -w)
else ()
	target_compile_options(ufbx PRIVATE /w)
endif ()

target_include_directories(${TARGET_BINARY_FOR_SETUP} SYSTEM PUBLIC ${UFBX_DIR})

target_link_libraries(${TARGET_BINARY_FOR_SETUP} PRIVATE ufbx)