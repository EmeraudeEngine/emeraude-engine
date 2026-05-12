if ( NOT TARGET_BINARY_FOR_SETUP )
	message(FATAL_ERROR "TARGET_BINARY_FOR_SETUP is not SET !")
endif ()

message("Enabling bc7enc_rdo BC7 texture compression library from local precompiled source ...")

find_package(bc7enc_rdo CONFIG REQUIRED PATHS ${LOCAL_LIB_DIR} NO_DEFAULT_PATH)

# Headers are at ${LOCAL_LIB_DIR}/include/bc7enc_rdo/, included via the imported target.
target_link_libraries(${TARGET_BINARY_FOR_SETUP} PRIVATE bc7enc_rdo::bc7enc_rdo)