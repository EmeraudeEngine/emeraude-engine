message("[EmeraudeEngine] Installing emeraude-base library ...")

set(EMERAUDE_BASE_GIT "https://github.com/EmeraudeEngine/emeraude-base.git")
set(EMERAUDE_BASE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/dependencies/emeraude-base)

if ( NOT EXISTS ${EMERAUDE_BASE_PATH} )
	find_package(Git REQUIRED)

	execute_process(
		COMMAND ${GIT_EXECUTABLE}
		clone --branch main --recurse-submodules ${EMERAUDE_BASE_GIT} ${EMERAUDE_BASE_PATH}
		COMMAND_ERROR_IS_FATAL ANY
	)
else ()
	message("[EmeraudeEngine] The emeraude-base repository is present !")
endif ()

add_subdirectory(${EMERAUDE_BASE_PATH} emeraude-base EXCLUDE_FROM_ALL)