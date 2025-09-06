if ( NOT TARGET_BINARY_FOR_SETUP )
	message(FATAL_ERROR "TARGET_BINARY_FOR_SETUP is not SET !")
endif ()

if ( EMERAUDE_USE_SYSTEM_LIBS )
	message("Enabling LZMA library from system ...")

	# NOTE: https://cmake.org/cmake/help/latest/module/FindLibLZMA.html
	find_package(LibLZMA REQUIRED)

	target_include_directories(${TARGET_BINARY_FOR_SETUP} PUBLIC ${LIBLZMA_INCLUDE_DIRS})

	target_link_directories(${TARGET_BINARY_FOR_SETUP} PUBLIC ${LIBLZMA_LIBRARIES})

	target_link_libraries(${TARGET_BINARY_FOR_SETUP} PRIVATE LibLZMA::LibLZMA)
else ()
	message("Enabling LZMA library from local source ...")

	target_compile_definitions(${TARGET_BINARY_FOR_SETUP} PUBLIC LZMA_API_STATIC)

	target_link_libraries(${TARGET_BINARY_FOR_SETUP} PRIVATE lzma)
endif ()
