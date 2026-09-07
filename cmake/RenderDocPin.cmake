# cmake/RenderDocPin.cmake
#
# Single source of truth for the RenderDoc revision used by BOTH consumers:
#   - SetupRenderDoc.cmake       : fetches the sources for the in-application API header.
#   - BuildRenderDocPython.cmake : clones the same revision to build the Python analysis module.
#
# Keeping the pin here is what guarantees that the header compiled into the engine and the
# 'renderdoc' Python module reading the .rdc captures come from the same RenderDoc version.
#
# Override on the command line with -DRENDERDOC_GIT_TAG=<tag> if a capture needs another release.

if ( NOT DEFINED RENDERDOC_GIT_REPOSITORY )
	set(RENDERDOC_GIT_REPOSITORY "https://github.com/baldurk/renderdoc.git")
endif ()

if ( NOT DEFINED RENDERDOC_GIT_TAG )
	set(RENDERDOC_GIT_TAG "v1.46")
endif ()
