if ( NOT TARGET_BINARY_FOR_SETUP )
	message(FATAL_ERROR "TARGET_BINARY_FOR_SETUP is not SET !")
endif ()

message("Enabling OpenMP library from system ...")

if ( MSVC )
	target_compile_options(${TARGET_BINARY_FOR_SETUP} PUBLIC /openmp:experimental)

	set(OPENMP_ENABLED On)
else ()
	find_package(OpenMP)

	if ( OpenMP_CXX_FOUND )
		# Must be PUBLIC
		target_link_libraries(${TARGET_BINARY_FOR_SETUP} PUBLIC OpenMP::OpenMP_CXX)

		target_compile_options(${TARGET_BINARY_FOR_SETUP} PUBLIC -fopenmp)

		set(OPENMP_ENABLED On)
	endif ()
endif ()


