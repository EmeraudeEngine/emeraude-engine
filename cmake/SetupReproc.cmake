if ( NOT TARGET_BINARY_FOR_SETUP )
	message(FATAL_ERROR "TARGET_BINARY_FOR_SETUP is not SET !")
endif ()

message("Enabling reproc/reproc++ library from local precompiled source ...")

find_package(reproc CONFIG REQUIRED PATHS ${EMERAUDE_EXT_LIBS_PATH} NO_DEFAULT_PATH)
find_package(reproc++ CONFIG REQUIRED PATHS ${EMERAUDE_EXT_LIBS_PATH} NO_DEFAULT_PATH)

# Headers are already included via ${EMERAUDE_EXT_LIBS_PATH}/include in the main CMakeLists.txt.
target_link_libraries(${TARGET_BINARY_FOR_SETUP} PRIVATE reproc reproc++)