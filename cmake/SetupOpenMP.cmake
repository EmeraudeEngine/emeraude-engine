if ( NOT TARGET_BINARY_FOR_SETUP )
	message(FATAL_ERROR "TARGET_BINARY_FOR_SETUP is not SET !")
endif ()

message("Enabling OpenMP library from system ...")

if ( MSVC )
	#find_package(OpenMP)

	#target_link_libraries(${TARGET_BINARY_FOR_SETUP} PUBLIC OpenMP::OpenMP_CXX)
	#target_compile_options(${TARGET_BINARY_FOR_SETUP} PUBLIC -openmp:experimental)
else ()
	find_package(OpenMP REQUIRED)

	target_link_libraries(${TARGET_BINARY_FOR_SETUP} PUBLIC OpenMP::OpenMP_CXX)
	target_compile_options(${TARGET_BINARY_FOR_SETUP} PUBLIC -fopenmp)
endif ()

set(OPENMP_ENABLED On)
