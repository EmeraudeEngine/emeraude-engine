message("Enabling ImGUI library from local source ...")

set(IMGUI_VERSION "1.9.7git")
set(IMGUI_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/dependencies/imgui)
set(IMGUI_INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/dependencies/imgui)

set(IMGUI_SOURCE_FILES
	${IMGUI_SOURCE_DIR}/imgui.cpp

	${IMGUI_SOURCE_DIR}/imgui_demo.cpp
	${IMGUI_SOURCE_DIR}/imgui_draw.cpp
	${IMGUI_SOURCE_DIR}/imgui_tables.cpp
	${IMGUI_SOURCE_DIR}/imgui_widgets.cpp

	${IMGUI_SOURCE_DIR}/backends/imgui_impl_glfw.cpp
	${IMGUI_SOURCE_DIR}/backends/imgui_impl_vulkan.cpp
)
