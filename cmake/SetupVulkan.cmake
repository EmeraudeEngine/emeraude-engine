if ( NOT VULKAN_ENABLED )
	if ( EMERAUDE_USE_SYSTEM_LIBS )
		message("Enabling Vulkan and GLSLang library from system ...")

		# https://cmake.org/cmake/help/latest/module/FindVulkan.html
		find_package(Vulkan REQUIRED COMPONENTS glslang glslangValidator SPIRV-Tools)

		message("Vulkan ${Vulkan_VERSION} library found !")
		message(" - Headers : ${Vulkan_INCLUDE_DIRS}")
		message(" - Binary : ${Vulkan_LIBRARIES}")
		message(" - SDK : ${VULKAN_SDK}")

		target_include_directories(${PROJECT_NAME} PRIVATE ${Vulkan_INCLUDE_DIRS})
		target_link_libraries(${PROJECT_NAME} PRIVATE ${Vulkan_LIBRARIES})

		if ( Vulkan_glslc_FOUND )
			message("Vulkan glslc found !")
			message(" - Executable : ${Vulkan_GLSLC_EXECUTABLE}")
		endif ()

		if ( Vulkan_glslangValidator_FOUND )
			message("Vulkan glslangValidator found !")
			message(" - Executable : ${Vulkan_GLSLANG_VALIDATOR_EXECUTABLE}")
		endif ()

		if ( Vulkan_dxc_exe_FOUND )
			message("Vulkan dxc executable found !")
			message(" - Executable : ${Vulkan_dxc_EXECUTABLE}")
		endif ()

		if ( Vulkan_glslang_FOUND )
			message("Vulkan glslang found !")
			message(" - Binary : ${Vulkan_glslang_LIBRARY}")

			#target_link_libraries(${PROJECT_NAME} PRIVATE ${Vulkan_glslang_LIBRARY} MachineIndependent OGLCompiler GenericCodeGen OSDependent glslang)
			target_link_libraries(${PROJECT_NAME} PRIVATE ${Vulkan_glslang_LIBRARY} MachineIndependent GenericCodeGen OSDependent glslang)
		endif ()

		if ( Vulkan_SPIRV-Tools_FOUND )
			message("Vulkan SPIRV-Tools found !")
			message(" - Binary : ${Vulkan_SPIRV-Tools_LIBRARY}")

			target_link_libraries(${PROJECT_NAME} PRIVATE ${Vulkan_SPIRV-Tools_LIBRARY} SPIRV SPIRV-Tools-opt SPIRV-Tools-reduce SPIRV-Tools-link SPIRV-Tools)
		endif ()

		if ( Vulkan_shaderc_combined_FOUND )
			message("Vulkan shaderc_combined found !")
			message(" - Binary : ${Vulkan_shaderc_combined_LIBRARY}")

			target_link_libraries(${PROJECT_NAME} PRIVATE ${Vulkan_shaderc_combined_LIBRARY})
		endif ()

		if ( Vulkan_MoltenVK_FOUND )
			message("Vulkan MoltenVK found !")
			message(" - Binary : ${Vulkan_MoltenVK_LIBRARY}")

			target_link_libraries(${PROJECT_NAME} PRIVATE ${Vulkan_MoltenVK_LIBRARY})
		endif ()

		if ( Vulkan_dxc_lib_FOUND )
			message("Vulkan dxc library found !")
			message(" - Binary : ${Vulkan_dxc_LIBRARY}")

			target_link_libraries(${PROJECT_NAME} PRIVATE ${Vulkan_dxc_LIBRARY})
		endif ()
	else ()
		message("Enabling Vulkan library from system ...")

		if ( MSVC )
			set(ENV{VULKAN_SDK} "C:/VulkanSDK/1.3.296.0/")
		endif ()

		find_package(Vulkan REQUIRED)

		message("Vulkan ${Vulkan_VERSION} library enabled !")
		message(" - Headers : ${Vulkan_INCLUDE_DIRS}")
		message(" - Binary : ${Vulkan_LIBRARIES}")

		target_include_directories(${PROJECT_NAME} PUBLIC ${Vulkan_INCLUDE_DIRS})
		target_link_libraries(${PROJECT_NAME} PUBLIC Vulkan::Vulkan)

		message("Configuring GLSLang library as sub-project ...")

		if ( NOT EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/dependencies/glslang/External/spirv-tools )
			message("Launching '${CMAKE_CURRENT_SOURCE_DIR}/dependencies/glslang/update_glslang_sources.py' ...")

			find_package(Python3 REQUIRED COMPONENTS Interpreter)

			execute_process(
				COMMAND ${Python3_EXECUTABLE} update_glslang_sources.py
				WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/dependencies/glslang
				COMMAND_ERROR_IS_FATAL ANY
			)
		endif ()

		set(GLSLANG_TESTS_DEFAULT Off CACHE BOOL "" FORCE)
		set(GLSLANG_ENABLE_INSTALL_DEFAULT Off CACHE BOOL "" FORCE)
		set(BUILD_SHARED_LIBS Off CACHE BOOL "" FORCE)
		set(ENABLE_GLSLANG_BINARIES Off CACHE BOOL "" FORCE)
		set(ENABLE_GLSLANG_JS Off CACHE BOOL "" FORCE)
		set(ENABLE_HLSL Off CACHE BOOL "" FORCE)
		set(ENABLE_RTTI On CACHE BOOL "" FORCE)
		set(ENABLE_EXCEPTIONS Off CACHE BOOL "" FORCE)

		add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/dependencies/glslang EXCLUDE_FROM_ALL)

		target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/dependencies/glslang)
		target_link_libraries(${PROJECT_NAME} PRIVATE SPIRV glslang)
	endif ()

	set(VULKAN_ENABLED On)
else ()
	message("The Vulkan library is already enabled.")
endif ()
