if ( NOT TARGET_BINARY_FOR_SETUP )
	message(FATAL_ERROR "TARGET_BINARY_FOR_SETUP is not SET !")
endif ()

message("Enabling ufbx FBX parsing library from local precompiled source ...")

find_package(ufbx CONFIG REQUIRED PATHS ${LOCAL_LIB_DIR} NO_DEFAULT_PATH)

# Headers are at ${LOCAL_LIB_DIR}/include/ufbx/, included via the imported target.
target_link_libraries(${TARGET_BINARY_FOR_SETUP} PRIVATE ufbx::ufbx)