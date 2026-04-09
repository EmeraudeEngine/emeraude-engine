if ( NOT TARGET_BINARY_FOR_SETUP )
	message(FATAL_ERROR "TARGET_BINARY_FOR_SETUP is not SET !")
endif ()

message("Configuring bc7enc_rdo BC7 texture compression library ...")

set(BC7ENC_RDO_DIR ${CMAKE_CURRENT_SOURCE_DIR}/dependencies/bc7enc_rdo)

# Build a static library from the minimal set of source files.
# NOTE: We only include the core encoder/decoder, not the RDO optimizer
# or utility files (lodepng, utils, test) which are not needed.
add_library(bc7enc_rdo STATIC
	${BC7ENC_RDO_DIR}/bc7enc.cpp
	${BC7ENC_RDO_DIR}/bc7decomp.cpp
)

target_include_directories(bc7enc_rdo SYSTEM PUBLIC ${BC7ENC_RDO_DIR})

# Suppress warnings from third-party code (compiled with -Werror in the engine).
# Optimization flags are inherited from the build type (Release/Debug) —
# do not force them here to avoid MSVC flag conflicts (e.g., /RTC1 vs /O2).
if ( NOT MSVC )
	target_compile_options(bc7enc_rdo PRIVATE -w)
else ()
	target_compile_options(bc7enc_rdo PRIVATE /w)
endif ()

target_include_directories(${TARGET_BINARY_FOR_SETUP} SYSTEM PUBLIC ${BC7ENC_RDO_DIR})

target_link_libraries(${TARGET_BINARY_FOR_SETUP} PRIVATE bc7enc_rdo)