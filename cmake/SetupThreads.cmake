if ( NOT TARGET_BINARY_FOR_SETUP )
	message(FATAL_ERROR "TARGET_BINARY_FOR_SETUP is not SET !")
endif ()

message("Enabling Threads library from system ...")

find_package(Threads REQUIRED)

if ( Threads_FOUND )
	message("Threads library found !")

	if ( CMAKE_USE_WIN32_THREADS_INIT )
		message("Using win32 thread !")
	endif ()

	if ( CMAKE_USE_PTHREADS_INIT )
		message("Using pthread !")
	endif ()

	if ( CMAKE_HP_PTHREADS_INIT )
		message("Using HP pthread !")
	endif ()

	target_link_libraries(${TARGET_BINARY_FOR_SETUP} PRIVATE Threads::Threads)
endif ()

set(THREADS_ENABLED On)
