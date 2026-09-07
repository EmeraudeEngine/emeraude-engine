if ( NOT TARGET_BINARY_FOR_SETUP )
    message(FATAL_ERROR "TARGET_BINARY_FOR_SETUP is not SET !")
endif ()

message("Configuring GLFW 3 framework as sub-project ...")

set(GLFW_BUILD_EXAMPLES Off)
set(GLFW_BUILD_TESTS Off)
set(GLFW_BUILD_DOCS Off)
set(GLFW_INSTALL Off)

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/dependencies/glfw EXCLUDE_FROM_ALL)

target_include_directories(${TARGET_BINARY_FOR_SETUP} SYSTEM PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/dependencies/glfw/include)

# Make every TU that pulls <GLFW/glfw3.h> see Vulkan extensions, without forcing
# the heavy emeraude_config.hpp include just for this define.
# GLFW_INCLUDE_NONE is mandatory alongside it: in glfw3.h the Vulkan include and the OpenGL
# include are two INDEPENDENT blocks, and only GLFW_INCLUDE_NONE suppresses the latter. Without
# it, every TU including <GLFW/glfw3.h> without a local '#define GLFW_INCLUDE_NONE' (Window.hpp,
# imgui_impl_glfw.cpp, ...) requires <GL/gl.h> — a build dependency on the OpenGL headers that
# this Vulkan-only engine has no use for (it broke the Linux CI once the GTK dev umbrella, which
# was pulling libgl-dev transitively, was removed). This is the canonical GLFW+Vulkan setup.
target_compile_definitions(${TARGET_BINARY_FOR_SETUP} PUBLIC GLFW_INCLUDE_VULKAN GLFW_INCLUDE_NONE)

target_link_libraries(${TARGET_BINARY_FOR_SETUP} PRIVATE glfw)
