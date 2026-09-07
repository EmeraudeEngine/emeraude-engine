if ( NOT TARGET_BINARY_FOR_SETUP )
	message(FATAL_ERROR "TARGET_BINARY_FOR_SETUP is not SET !")
endif ()

message("Enabling reproc/reproc++ library from local precompiled source ...")

# NOTE: This find_package() is mandatory even though only reproc++ is linked below.
# reproc++-config.cmake does call find_dependency(reproc), but without PATHS/NO_DEFAULT_PATH,
# so it cannot reach the ext-deps prefix on its own; find_dependency() simply sees the target
# as already found thanks to this explicit call.
find_package(reproc CONFIG REQUIRED PATHS ${EMERAUDE_EXT_LIBS_PATH} NO_DEFAULT_PATH)
find_package(reproc++ CONFIG REQUIRED PATHS ${EMERAUDE_EXT_LIBS_PATH} NO_DEFAULT_PATH)

# Headers are already included via ${EMERAUDE_EXT_LIBS_PATH}/include in the main CMakeLists.txt.

# NOTE: Only reproc++ is consumed by the engine sources (reproc++/run.hpp, in
# Desktop/Commands.{linux.cpp,mac.mm}); the C library comes along with it through
# $<LINK_ONLY:reproc> in reproc++'s exported config, placed after libreproc++.a as static
# linking requires. Naming `reproc` here as well, and before reproc++, conflicted with that
# ordering: CMake honoured the requested position AND re-emitted the entry at the end of the
# link line, so libreproc.a appeared twice and Apple's linker reported
# "ld: warning: ignoring duplicate libraries".
target_link_libraries(${TARGET_BINARY_FOR_SETUP} PRIVATE reproc++)
