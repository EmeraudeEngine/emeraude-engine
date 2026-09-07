if ( NOT TARGET_BINARY_FOR_SETUP )
	message(FATAL_ERROR "TARGET_BINARY_FOR_SETUP is not SET !")
endif ()

# RenderDoc is NOT a submodule (2026-09-07): nothing lands on disk unless EMERAUDE_ENABLE_RENDERDOC
# is ON, and this file is the only place that pulls it. The engine consumes exactly ONE file from
# that tree, the MIT-licensed 'renderdoc_app.h' in-application API header
# (https://github.com/baldurk/renderdoc, MIT, by Baldur Karlsson) — there is no library to link
# against, the RenderDoc runtime is detected through the Vulkan capture layer at run time.

include(${CMAKE_CURRENT_LIST_DIR}/RenderDocPin.cmake)

get_filename_component(RENDERDOC_LOCAL_CHECKOUT "${CMAKE_CURRENT_LIST_DIR}/../dependencies/renderdoc" ABSOLUTE)

# A checkout already sitting at the former submodule path is reused as-is, so the fetch does not
# duplicate ~219 MB in every build directory, and so the tree that carries the built Python
# analysis module (build/lib/renderdoc.so) stays the one being used.
#
# ⚠️⚠️ This reuse MUST go through FETCHCONTENT_SOURCE_DIR_<UPPERCASE NAME>, the only mechanism that
# makes FetchContent skip the download step altogether. Passing that same path as SOURCE_DIR would
# instead hand the directory to the ExternalProject git-clone step, which ERASES its source
# directory whenever the stamp file is missing — that is, from every freshly created build
# directory. It would take the RenderDoc build/ directory (240 MB, minutes of compilation) with it.
if ( FETCHCONTENT_SOURCE_DIR_RENDERDOC AND NOT EXISTS "${FETCHCONTENT_SOURCE_DIR_RENDERDOC}/renderdoc/api/app/renderdoc_app.h" )
	message("[EmeraudeEngine] RenderDoc: '${FETCHCONTENT_SOURCE_DIR_RENDERDOC}' no longer holds the sources, falling back to the download.")

	unset(FETCHCONTENT_SOURCE_DIR_RENDERDOC CACHE)
	unset(FETCHCONTENT_SOURCE_DIR_RENDERDOC)
endif ()

if ( NOT FETCHCONTENT_SOURCE_DIR_RENDERDOC AND EXISTS "${RENDERDOC_LOCAL_CHECKOUT}/renderdoc/api/app/renderdoc_app.h" )
	set(FETCHCONTENT_SOURCE_DIR_RENDERDOC "${RENDERDOC_LOCAL_CHECKOUT}" CACHE PATH "Existing RenderDoc checkout to use instead of downloading the sources.")
endif ()

if ( FETCHCONTENT_SOURCE_DIR_RENDERDOC )
	message("[EmeraudeEngine] Enabling RenderDoc in-application API (local sources: ${FETCHCONTENT_SOURCE_DIR_RENDERDOC}) ...")
else ()
	message("[EmeraudeEngine] Enabling RenderDoc in-application API (fetching ${RENDERDOC_GIT_REPOSITORY} @ ${RENDERDOC_GIT_TAG}, ~219 MB) ...")
endif ()

include(FetchContent)

FetchContent_Declare(renderdoc
	GIT_REPOSITORY ${RENDERDOC_GIT_REPOSITORY}
	GIT_TAG ${RENDERDOC_GIT_TAG}
	GIT_SHALLOW On
	GIT_PROGRESS On
	# Download only. RenderDoc's own CMake project must NEVER enter our build tree (it would build
	# the capture library, qrenderdoc and their dependencies). Pointing SOURCE_SUBDIR at a
	# directory that holds no CMakeLists.txt is the documented way to make FetchContent_MakeAvailable()
	# populate the sources without calling add_subdirectory() on them.
	SOURCE_SUBDIR "emeraude-header-only-no-cmakelists-here"
)

FetchContent_MakeAvailable(renderdoc)

set(RENDERDOC_INCLUDE_DIR "${renderdoc_SOURCE_DIR}/renderdoc/api/app")

if ( NOT EXISTS "${RENDERDOC_INCLUDE_DIR}/renderdoc_app.h" )
	message(FATAL_ERROR "[EmeraudeEngine] RenderDoc sources are at '${renderdoc_SOURCE_DIR}' but 'renderdoc/api/app/renderdoc_app.h' is missing ! Check RENDERDOC_GIT_TAG (${RENDERDOC_GIT_TAG}).")
endif ()

target_include_directories(${TARGET_BINARY_FOR_SETUP} SYSTEM PUBLIC ${RENDERDOC_INCLUDE_DIR})
target_compile_definitions(${TARGET_BINARY_FOR_SETUP} PUBLIC EMERAUDE_ENABLE_RENDERDOC)

if ( UNIX )
	target_link_libraries(${TARGET_BINARY_FOR_SETUP} PUBLIC ${CMAKE_DL_LIBS})
endif ()

set(RENDERDOC_ENABLED On)
