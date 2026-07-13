if ( TARGET emeraude::base )
	message("[EmeraudeEngine] emeraude-base is already provided by a parent project, reusing it !")
	return()
endif ()

message("[EmeraudeEngine] Installing emeraude-base library ...")

set(EMERAUDE_BASE_GIT "https://github.com/EmeraudeEngine/emeraude-base.git")
# Branch cloned ONLY when dependencies/emeraude-base is absent (a fresh clone-based
# build, e.g. CI). When the directory already exists — a local symlink or checkout —
# it is master and nothing is cloned, so this is ignored. Default main; overridable
# (e.g. -DEMERAUDE_BASE_GIT_BRANCH=develop) to track base changes not yet on main.
set(EMERAUDE_BASE_GIT_BRANCH "main" CACHE STRING "emeraude-base branch to clone when the dependency is absent.")
set(EMERAUDE_BASE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/dependencies/emeraude-base)

if ( NOT EXISTS ${EMERAUDE_BASE_PATH} )
	find_package(Git REQUIRED)

	execute_process(
		COMMAND ${GIT_EXECUTABLE}
		clone --branch ${EMERAUDE_BASE_GIT_BRANCH} --recurse-submodules ${EMERAUDE_BASE_GIT} ${EMERAUDE_BASE_PATH}
		COMMAND_ERROR_IS_FATAL ANY
	)
else ()
	message("[EmeraudeEngine] The emeraude-base repository is present !")
endif ()

add_subdirectory(${EMERAUDE_BASE_PATH} emeraude-base EXCLUDE_FROM_ALL)